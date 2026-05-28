import { common } from '@kit.AbilityKit'
import { preferences } from '@kit.ArkData'
import { util } from '@kit.ArkTS'
import { asset } from '@kit.AssetStoreKit'
import { RdpLogger } from './RdpLogger'

const STORE_NAME = 'windowsConnectionProfiles'
const KEY_PROFILE_IDS = 'profileIds'
const KEY_DEFAULT_PROFILE_ID = 'defaultProfileId'
const KEY_PROFILE_PREFIX = 'profile.'
const PASSWORD_ALIAS_PREFIX = 'muhub.rdp.password.'
const DEFAULT_RDP_PORT = '3389'
const MAX_PROFILES = 20

export interface WindowsConnectionProfile {
  id: string
  name: string
  host: string
  port: string
  username: string
  certPolicy: string
  rememberPassword: boolean
  passwordAlias: string
  createdAt: number
  updatedAt: number
  lastConnectedAt: number
}

export interface WindowsConnectionSaveInput {
  profileId: string
  host: string
  port: string
  username: string
  password: string
  certPolicy: string
  rememberPassword: boolean
}

export interface WindowsConnectionSnapshot {
  profiles: WindowsConnectionProfile[]
  selectedProfileId: string
  selectedPassword: string
}

export class WindowsConnectionStore {
  private readonly context: common.Context
  private preferencesCache: preferences.Preferences | null = null

  constructor(context: common.Context) {
    this.context = context
  }

  async loadSnapshot(): Promise<WindowsConnectionSnapshot> {
    const profiles = await this.loadProfiles()
    const selectedProfileId = await this.resolveSelectedProfileId(profiles)
    const selectedProfile = WindowsConnectionStore.findProfileById(profiles, selectedProfileId)
    const selectedPassword = selectedProfile === null ? '' : await this.loadPasswordForProfile(selectedProfile)
    return {
      profiles: profiles,
      selectedProfileId: selectedProfileId,
      selectedPassword: selectedPassword
    }
  }

  async loadPassword(profile: WindowsConnectionProfile): Promise<string> {
    return this.loadPasswordForProfile(profile)
  }

  async saveProfile(input: WindowsConnectionSaveInput): Promise<WindowsConnectionProfile | null> {
    const host = input.host.trim()
    const port = WindowsConnectionStore.normalizePort(input.port)
    const username = input.username.trim()
    if (host.length === 0 || username.length === 0) {
      return null
    }

    const now = Date.now()
    const profiles = await this.loadProfiles()
    const existing = WindowsConnectionStore.findProfileForSave(profiles, input.profileId, host, port, username)
    const profile = existing === null ?
      WindowsConnectionStore.createProfile(host, port, username, input.certPolicy, now) :
      WindowsConnectionStore.updatedProfile(existing, host, port, username, input.certPolicy, now)

    await this.applyPasswordPreference(profile, input.rememberPassword, input.password)
    const nextProfiles = WindowsConnectionStore.upsertProfile(profiles, profile)
    await this.persistProfiles(nextProfiles, profile.id)
    return profile
  }

  async deleteProfile(profileId: string): Promise<WindowsConnectionSnapshot> {
    const profiles = await this.loadProfiles()
    const target = WindowsConnectionStore.findProfileById(profiles, profileId)
    if (target !== null) {
      await this.removePassword(target.passwordAlias)
    }

    const nextProfiles = profiles.filter((profile: WindowsConnectionProfile): boolean => profile.id !== profileId)
    await this.persistProfiles(nextProfiles, WindowsConnectionStore.firstProfileId(nextProfiles))
    await this.deleteProfileKey(profileId)
    return this.loadSnapshot()
  }

  async clearPassword(profileId: string): Promise<WindowsConnectionSnapshot> {
    const profiles = await this.loadProfiles()
    const target = WindowsConnectionStore.findProfileById(profiles, profileId)
    if (target === null) {
      return this.loadSnapshot()
    }

    await this.removePassword(target.passwordAlias)
    target.rememberPassword = false
    target.passwordAlias = ''
    target.updatedAt = Date.now()
    await this.persistProfiles(WindowsConnectionStore.upsertProfile(profiles, target), target.id)
    return this.loadSnapshot()
  }

  private async getPreferences(): Promise<preferences.Preferences | null> {
    if (this.preferencesCache !== null) {
      return this.preferencesCache
    }

    try {
      this.preferencesCache = await preferences.getPreferences(this.context, STORE_NAME)
      return this.preferencesCache
    } catch (error) {
      RdpLogger.error(`windows connection preferences open failed: ${JSON.stringify(error)}`)
      return null
    }
  }

  private async loadProfiles(): Promise<WindowsConnectionProfile[]> {
    const store = await this.getPreferences()
    if (store === null) {
      return []
    }

    const profileIds = await this.loadProfileIds(store)
    const profiles: WindowsConnectionProfile[] = []
    for (const profileId of profileIds) {
      const profile = await this.loadProfile(store, profileId)
      if (profile !== null) {
        profiles.push(profile)
      }
    }
    return WindowsConnectionStore.sortProfiles(profiles)
  }

  private async loadProfileIds(store: preferences.Preferences): Promise<string[]> {
    try {
      const raw = await store.get(KEY_PROFILE_IDS, '[]')
      if (typeof raw !== 'string') {
        return []
      }
      const parsed = JSON.parse(raw) as string[]
      if (!Array.isArray(parsed)) {
        return []
      }
      return parsed.filter((profileId: string): boolean => typeof profileId === 'string' && profileId.length > 0)
    } catch (error) {
      RdpLogger.warn(`windows connection profile ids read failed: ${JSON.stringify(error)}`)
      return []
    }
  }

  private async loadProfile(store: preferences.Preferences, profileId: string): Promise<WindowsConnectionProfile | null> {
    try {
      const raw = await store.get(WindowsConnectionStore.profileKey(profileId), '')
      if (typeof raw !== 'string' || raw.length === 0) {
        return null
      }
      return WindowsConnectionStore.sanitizeProfile(JSON.parse(raw) as WindowsConnectionProfile)
    } catch (error) {
      RdpLogger.warn(`windows connection profile read failed: id=${profileId}, error=${JSON.stringify(error)}`)
      return null
    }
  }

  private async resolveSelectedProfileId(profiles: WindowsConnectionProfile[]): Promise<string> {
    if (profiles.length === 0) {
      return ''
    }

    const store = await this.getPreferences()
    if (store !== null) {
      try {
        const raw = await store.get(KEY_DEFAULT_PROFILE_ID, '')
        if (typeof raw === 'string' && WindowsConnectionStore.findProfileById(profiles, raw) !== null) {
          return raw
        }
      } catch (error) {
        RdpLogger.warn(`windows connection selected profile read failed: ${JSON.stringify(error)}`)
      }
    }
    return profiles[0].id
  }

  private async persistProfiles(profiles: WindowsConnectionProfile[], selectedProfileId: string): Promise<void> {
    const store = await this.getPreferences()
    if (store === null) {
      return
    }

    const sortedProfiles = WindowsConnectionStore.sortProfiles(profiles)
    const profilesToKeep = sortedProfiles.slice(0, MAX_PROFILES)
    const removedProfiles = sortedProfiles.slice(MAX_PROFILES)

    try {
      const profileIds = profilesToKeep.map((profile: WindowsConnectionProfile): string => profile.id)
      await store.put(KEY_PROFILE_IDS, JSON.stringify(profileIds))
      await store.put(KEY_DEFAULT_PROFILE_ID,
        WindowsConnectionStore.findProfileById(profilesToKeep, selectedProfileId) === null ?
          WindowsConnectionStore.firstProfileId(profilesToKeep) : selectedProfileId)

      for (const profile of profilesToKeep) {
        await store.put(WindowsConnectionStore.profileKey(profile.id), JSON.stringify(profile))
      }
      for (const profile of removedProfiles) {
        await store.delete(WindowsConnectionStore.profileKey(profile.id))
        await this.removePassword(profile.passwordAlias)
      }
      await store.flush()
    } catch (error) {
      RdpLogger.error(`windows connection profiles persist failed: ${JSON.stringify(error)}`)
    }
  }

  private async deleteProfileKey(profileId: string): Promise<void> {
    const store = await this.getPreferences()
    if (store === null) {
      return
    }
    try {
      await store.delete(WindowsConnectionStore.profileKey(profileId))
      await store.flush()
    } catch (error) {
      RdpLogger.warn(`windows connection profile key delete failed: id=${profileId}, error=${JSON.stringify(error)}`)
    }
  }

  private async applyPasswordPreference(profile: WindowsConnectionProfile, rememberPassword: boolean,
    password: string): Promise<void> {
    const passwordAlias = WindowsConnectionStore.passwordAlias(profile.id)
    if (!rememberPassword) {
      await this.removePassword(profile.passwordAlias.length > 0 ? profile.passwordAlias : passwordAlias)
      profile.rememberPassword = false
      profile.passwordAlias = ''
      return
    }

    if (password.length > 0) {
      const saved = await this.savePassword(passwordAlias, password)
      profile.rememberPassword = saved
      profile.passwordAlias = saved ? passwordAlias : ''
      return
    }

    profile.rememberPassword = profile.passwordAlias.length > 0
  }

  private async savePassword(alias: string, password: string): Promise<boolean> {
    try {
      const attributes: asset.AssetMap = new Map()
      attributes.set(asset.Tag.ALIAS, WindowsConnectionStore.stringToBytes(alias))
      attributes.set(asset.Tag.SECRET, WindowsConnectionStore.stringToBytes(password))
      attributes.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_FIRST_UNLOCKED)
      attributes.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE)
      attributes.set(asset.Tag.SYNC_TYPE, asset.SyncType.NEVER)
      attributes.set(asset.Tag.CONFLICT_RESOLUTION, asset.ConflictResolution.OVERWRITE)
      await asset.add(attributes)
      return true
    } catch (error) {
      RdpLogger.warn(`windows connection password save failed: alias=${alias}, error=${JSON.stringify(error)}`)
      return false
    }
  }

  private async loadPasswordForProfile(profile: WindowsConnectionProfile): Promise<string> {
    if (!profile.rememberPassword || profile.passwordAlias.length === 0) {
      return ''
    }

    try {
      const query: asset.AssetMap = new Map()
      query.set(asset.Tag.ALIAS, WindowsConnectionStore.stringToBytes(profile.passwordAlias))
      query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      const result = await asset.query(query)
      if (result.length === 0) {
        return ''
      }
      const secret = result[0].get(asset.Tag.SECRET)
      return secret instanceof Uint8Array ? WindowsConnectionStore.bytesToString(secret) : ''
    } catch (error) {
      RdpLogger.warn(`windows connection password load failed: alias=${profile.passwordAlias}, error=${JSON.stringify(error)}`)
      return ''
    }
  }

  private async removePassword(alias: string): Promise<void> {
    if (alias.length === 0) {
      return
    }

    try {
      const query: asset.AssetMap = new Map()
      query.set(asset.Tag.ALIAS, WindowsConnectionStore.stringToBytes(alias))
      await asset.remove(query)
    } catch (error) {
      RdpLogger.warn(`windows connection password remove skipped: alias=${alias}, error=${JSON.stringify(error)}`)
    }
  }

  private static createProfile(host: string, port: string, username: string, certPolicy: string,
    now: number): WindowsConnectionProfile {
    const id = WindowsConnectionStore.newProfileId()
    return {
      id: id,
      name: WindowsConnectionStore.profileName(host, username),
      host: host,
      port: port,
      username: username,
      certPolicy: certPolicy,
      rememberPassword: false,
      passwordAlias: '',
      createdAt: now,
      updatedAt: now,
      lastConnectedAt: now
    }
  }

  private static updatedProfile(profile: WindowsConnectionProfile, host: string, port: string, username: string,
    certPolicy: string, now: number): WindowsConnectionProfile {
    profile.name = profile.name.length > 0 ? profile.name : WindowsConnectionStore.profileName(host, username)
    profile.host = host
    profile.port = port
    profile.username = username
    profile.certPolicy = certPolicy
    profile.updatedAt = now
    profile.lastConnectedAt = now
    return profile
  }

  private static sanitizeProfile(profile: WindowsConnectionProfile): WindowsConnectionProfile | null {
    if (!profile || typeof profile.id !== 'string' || typeof profile.host !== 'string' ||
      typeof profile.username !== 'string') {
      return null
    }

    const host = profile.host.trim()
    const username = profile.username.trim()
    if (profile.id.length === 0 || host.length === 0 || username.length === 0) {
      return null
    }

    return {
      id: profile.id,
      name: typeof profile.name === 'string' && profile.name.length > 0 ?
        profile.name : WindowsConnectionStore.profileName(host, username),
      host: host,
      port: WindowsConnectionStore.normalizePort(profile.port),
      username: username,
      certPolicy: typeof profile.certPolicy === 'string' && profile.certPolicy.length > 0 ? profile.certPolicy : 'tofu',
      rememberPassword: profile.rememberPassword === true,
      passwordAlias: typeof profile.passwordAlias === 'string' ? profile.passwordAlias : '',
      createdAt: WindowsConnectionStore.safeTimestamp(profile.createdAt),
      updatedAt: WindowsConnectionStore.safeTimestamp(profile.updatedAt),
      lastConnectedAt: WindowsConnectionStore.safeTimestamp(profile.lastConnectedAt)
    }
  }

  private static findProfileForSave(profiles: WindowsConnectionProfile[], profileId: string, host: string,
    port: string, username: string): WindowsConnectionProfile | null {
    const selected = WindowsConnectionStore.findProfileById(profiles, profileId)
    if (selected !== null) {
      return selected
    }
    for (const profile of profiles) {
      if (profile.host === host && profile.port === port && profile.username === username) {
        return profile
      }
    }
    return null
  }

  private static findProfileById(profiles: WindowsConnectionProfile[], profileId: string): WindowsConnectionProfile | null {
    for (const profile of profiles) {
      if (profile.id === profileId) {
        return profile
      }
    }
    return null
  }

  private static upsertProfile(profiles: WindowsConnectionProfile[],
    nextProfile: WindowsConnectionProfile): WindowsConnectionProfile[] {
    const nextProfiles = profiles.filter((profile: WindowsConnectionProfile): boolean => profile.id !== nextProfile.id)
    nextProfiles.push(nextProfile)
    return nextProfiles
  }

  private static sortProfiles(profiles: WindowsConnectionProfile[]): WindowsConnectionProfile[] {
    return profiles.sort((left: WindowsConnectionProfile, right: WindowsConnectionProfile): number => {
      if (right.lastConnectedAt !== left.lastConnectedAt) {
        return right.lastConnectedAt - left.lastConnectedAt
      }
      return right.updatedAt - left.updatedAt
    })
  }

  private static firstProfileId(profiles: WindowsConnectionProfile[]): string {
    return profiles.length > 0 ? profiles[0].id : ''
  }

  private static normalizePort(port: string): string {
    const value = typeof port === 'string' ? port.trim() : ''
    return value.length > 0 ? value : DEFAULT_RDP_PORT
  }

  private static profileName(host: string, username: string): string {
    return username.length > 0 ? `${username} @ ${host}` : host
  }

  private static profileKey(profileId: string): string {
    return `${KEY_PROFILE_PREFIX}${profileId}`
  }

  private static passwordAlias(profileId: string): string {
    return `${PASSWORD_ALIAS_PREFIX}${profileId}`
  }

  private static newProfileId(): string {
    return `${Date.now()}-${Math.floor(Math.random() * 1000000)}`
  }

  private static safeTimestamp(value: number): number {
    return typeof value === 'number' && Number.isFinite(value) && value > 0 ? value : Date.now()
  }

  private static stringToBytes(value: string): Uint8Array {
    return new util.TextEncoder().encodeInto(value)
  }

  private static bytesToString(value: Uint8Array): string {
    return new util.TextDecoder().decodeToString(value)
  }
}
