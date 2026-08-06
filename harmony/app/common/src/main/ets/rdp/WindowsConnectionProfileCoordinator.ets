import { common } from '@kit.AbilityKit'
import { RdpLogger } from './RdpLogger'
import {
  WindowsConnectionProfile,
  WindowsConnectionSaveInput,
  WindowsConnectionSnapshot,
  WindowsConnectionStore
} from './WindowsConnectionStore'

export class WindowsConnectionProfileCoordinator {
  private readonly store: WindowsConnectionStore

  constructor(context: common.Context) {
    this.store = new WindowsConnectionStore(context)
  }

  async load(): Promise<WindowsConnectionSnapshot | null> {
    try {
      return await this.store.loadSnapshot()
    } catch (error) {
      RdpLogger.warn(`windows connection profiles load failed: ${JSON.stringify(error)}`)
      return null
    }
  }

  async loadPassword(profile: WindowsConnectionProfile): Promise<string | null> {
    try {
      return await this.store.loadPassword(profile)
    } catch (error) {
      RdpLogger.warn(`windows connection profile password select failed: ${JSON.stringify(error)}`)
      return null
    }
  }

  async deleteProfile(profileId: string): Promise<WindowsConnectionSnapshot | null> {
    try {
      return await this.store.deleteProfile(profileId)
    } catch (error) {
      RdpLogger.warn(`windows connection profile delete failed: ${JSON.stringify(error)}`)
      return null
    }
  }

  async saveProfile(input: WindowsConnectionSaveInput): Promise<WindowsConnectionSnapshot | null> {
    try {
      const profile = await this.store.saveProfile(input)
      return profile === null ? null : await this.store.loadSnapshot()
    } catch (error) {
      RdpLogger.warn(`windows connection profile save failed: ${JSON.stringify(error)}`)
      return null
    }
  }

  async clearPassword(profileId: string): Promise<WindowsConnectionSnapshot | null> {
    try {
      return await this.store.clearPassword(profileId)
    } catch (error) {
      RdpLogger.warn(`windows connection password clear failed: ${JSON.stringify(error)}`)
      return null
    }
  }
}
