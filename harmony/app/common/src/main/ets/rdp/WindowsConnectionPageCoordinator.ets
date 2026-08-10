import { common } from '@kit.AbilityKit'
import { SettingsStatusTone } from '../components/settings/SettingsUi'
import { HomeText } from '../components/home/HomeText'
import {
  WindowsConnectionProfile,
  WindowsConnectionSaveInput,
  WindowsConnectionSnapshot
} from './WindowsConnectionStore'
import { WindowsConnectionProfileCoordinator } from './WindowsConnectionProfileCoordinator'

export interface WindowsConnectionPageDelegate {
  context: () => common.UIAbilityContext | null
  defer: (task: () => void) => void
  cancelEndpointProbe: () => void
  applySnapshot: (snapshot: WindowsConnectionSnapshot, applySelectedProfile: boolean, clearWhenEmpty: boolean) => void
  applyProfile: (profile: WindowsConnectionProfile, password: string) => void
  startNewProfile: () => void
  setPasswordLoading: (loading: boolean) => void
  setFeedback: (text: string, tone: SettingsStatusTone) => void
}

export class WindowsConnectionPageCoordinator {
  private coordinator: WindowsConnectionProfileCoordinator | null = null
  private profiles: WindowsConnectionProfile[] = []
  private selectionGeneration: number = 0
  private loaded: boolean = false
  private readonly delegate: WindowsConnectionPageDelegate

  constructor(delegate: WindowsConnectionPageDelegate) {
    this.delegate = delegate
  }

  isLoaded(): boolean {
    return this.loaded
  }

  private getCoordinator(): WindowsConnectionProfileCoordinator | null {
    if (this.coordinator !== null) {
      return this.coordinator
    }
    const context = this.delegate.context()
    if (context === null) {
      return null
    }
    this.coordinator = new WindowsConnectionProfileCoordinator(context)
    return this.coordinator
  }

  private remember(snapshot: WindowsConnectionSnapshot): void {
    this.profiles = snapshot.profiles
  }

  load(): void {
    const coordinator = this.getCoordinator()
    if (coordinator === null) {
      return
    }
    coordinator.load().then((snapshot: WindowsConnectionSnapshot | null): void => {
      if (snapshot === null) {
        return
      }
      this.delegate.defer((): void => {
        this.loaded = true
        this.remember(snapshot)
        this.delegate.applySnapshot(snapshot, true, false)
      })
    })
  }

  private find(profileId: string): WindowsConnectionProfile | null {
    for (const profile of this.profiles) {
      if (profile.id === profileId) {
        return profile
      }
    }
    return null
  }

  select(profileId: string): void {
    this.delegate.cancelEndpointProbe()
    if (profileId.length === 0) {
      this.startNew()
      return
    }
    const profile = this.find(profileId)
    if (profile === null) {
      return
    }
    const generation = ++this.selectionGeneration
    if (!profile.rememberPassword) {
      this.delegate.setPasswordLoading(false)
      this.delegate.applyProfile(profile, '')
      return
    }
    const coordinator = this.getCoordinator()
    if (coordinator === null) {
      this.delegate.setPasswordLoading(false)
      this.delegate.applyProfile(profile, '')
      return
    }
    this.delegate.setPasswordLoading(true)
    coordinator.loadPassword(profile).then((password: string | null): void => {
      this.delegate.defer((): void => {
        if (this.selectionGeneration !== generation) {
          return
        }
        this.delegate.applyProfile(profile, password === null ? '' : password)
        this.delegate.setPasswordLoading(false)
      })
    })
  }

  startNew(): void {
    this.delegate.cancelEndpointProbe()
    this.selectionGeneration++
    this.delegate.setPasswordLoading(false)
    this.delegate.startNewProfile()
  }

  deleteSelected(profileId: string): void {
    if (profileId.length === 0) {
      this.startNew()
      return
    }
    const coordinator = this.getCoordinator()
    if (coordinator === null) {
      return
    }
    coordinator.deleteProfile(profileId).then((snapshot: WindowsConnectionSnapshot | null): void => {
      if (snapshot === null) {
        return
      }
      this.delegate.defer((): void => {
        this.remember(snapshot)
        this.delegate.applySnapshot(snapshot, true, true)
      })
    })
  }

  save(input: WindowsConnectionSaveInput): void {
    const coordinator = this.getCoordinator()
    if (coordinator === null) {
      return
    }
    coordinator.saveProfile(input).then((snapshot: WindowsConnectionSnapshot | null): void => {
      if (snapshot === null) {
        return
      }
      this.delegate.defer((): void => {
        this.remember(snapshot)
        this.delegate.applySnapshot(snapshot, false, false)
      })
    })
  }

  clearPassword(profileId: string): void {
    if (profileId.length === 0) {
      this.delegate.setFeedback(HomeText.PASSWORD_CLEARED_CURRENT, 'neutral')
      return
    }
    const coordinator = this.getCoordinator()
    if (coordinator === null) {
      this.delegate.setFeedback(HomeText.PASSWORD_CLEAR_STORAGE_UNAVAILABLE, 'danger')
      return
    }
    coordinator.clearPassword(profileId).then((snapshot: WindowsConnectionSnapshot | null): void => {
      this.delegate.defer((): void => {
        if (snapshot === null) {
          this.delegate.setFeedback(HomeText.PASSWORD_CLEAR_FAILED, 'danger')
          return
        }
        this.remember(snapshot)
        this.delegate.applySnapshot(snapshot, true, false)
        this.delegate.setFeedback(HomeText.PASSWORD_CLEARED_SAVED, 'ok')
      })
    })
  }
}
