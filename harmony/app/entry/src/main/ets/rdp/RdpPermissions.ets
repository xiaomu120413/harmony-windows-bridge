import { abilityAccessCtrl, bundleManager, common, Permissions } from '@kit.AbilityKit'
import {
  APPROXIMATE_LOCATION_PERMISSION,
  LOCATION_PERMISSION,
  MICROPHONE_PERMISSION,
  READ_PASTEBOARD_PERMISSION,
  SCREEN_RECORDING_PERMISSION
} from './RdpConstants'
import { RdpLogger } from './RdpLogger'

export interface RdpPermissionDelegate {
  getContext(): common.UIAbilityContext | null
}

export class RdpPermissionCallbackDelegate implements RdpPermissionDelegate {
  private readonly getContextCallback: () => common.UIAbilityContext | null

  constructor(getContext: () => common.UIAbilityContext | null) {
    this.getContextCallback = getContext
  }

  getContext(): common.UIAbilityContext | null {
    return this.getContextCallback()
  }
}

interface PermissionRequestState {
  request: Promise<boolean> | null
  requested: boolean
}

type PermissionGrantChecker = () => boolean
type PermissionRequestFactory = (context: common.UIAbilityContext) => Promise<boolean>
type PermissionRequestResolver = (granted: boolean) => void

const SCREEN_RECORDING_PERMISSION_PROMPT_DELAY_MS: number = 3000

export class RdpPermissionManager {
  private delegate: RdpPermissionDelegate
  private pasteboardPermission: PermissionRequestState = { request: null, requested: false }
  private microphonePermission: PermissionRequestState = { request: null, requested: false }
  private locationPermission: PermissionRequestState = { request: null, requested: false }
  private screenRecordingPermission: PermissionRequestState = { request: null, requested: false }
  private screenRecordingPermissionPromptDelayTimer: number | null = null
  private screenRecordingPermissionPromptDelayResolve: PermissionRequestResolver | null = null

  constructor(delegate: RdpPermissionDelegate) {
    this.delegate = delegate
  }

  ensurePasteboardReadPermission(trigger: string): Promise<boolean> {
    return this.ensurePermission(
      this.pasteboardPermission,
      'Pasteboard read',
      trigger,
      (): boolean => this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard'),
      (context: common.UIAbilityContext): Promise<boolean> =>
        this.requestRuntimePermissions(context, 'Pasteboard read', [READ_PASTEBOARD_PERMISSION],
          (): boolean => this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard')),
      true
    )
  }

  ensureMicrophonePermission(trigger: string): Promise<boolean> {
    return this.ensurePermission(
      this.microphonePermission,
      'Microphone',
      trigger,
      (): boolean => this.hasPermission(MICROPHONE_PERMISSION, 'Microphone'),
      (context: common.UIAbilityContext): Promise<boolean> =>
        this.requestRuntimePermissions(context, 'Microphone', [MICROPHONE_PERMISSION],
          (): boolean => this.hasPermission(MICROPHONE_PERMISSION, 'Microphone')),
      false
    )
  }

  ensureLocationPermission(trigger: string): Promise<boolean> {
    const permissions: Permissions[] = [APPROXIMATE_LOCATION_PERMISSION, LOCATION_PERMISSION]
    return this.ensurePermission(
      this.locationPermission,
      'Location',
      trigger,
      (): boolean => this.hasLocationPermission(),
      (context: common.UIAbilityContext): Promise<boolean> =>
        this.requestRuntimePermissions(context, 'Location', permissions,
          (): boolean => this.hasLocationPermission()),
      false
    )
  }

  ensureScreenRecordingPermission(trigger: string): Promise<boolean> {
    return this.ensurePermission(
      this.screenRecordingPermission,
      'Screen recording',
      trigger,
      (): boolean => this.hasScreenRecordingPermission(),
      (context: common.UIAbilityContext): Promise<boolean> =>
        this.openScreenRecordingPermissionOnSettingAfterDelay(context),
      true
    )
  }

  requestScreenRecordingPermission(trigger: string): Promise<boolean> {
    if (this.hasScreenRecordingPermission()) {
      RdpLogger.info(`Screen recording permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (this.screenRecordingPermission.request !== null &&
      this.screenRecordingPermissionPromptDelayTimer !== null) {
      this.openPendingScreenRecordingPermissionPrompt(trigger)
      return this.screenRecordingPermission.request
    }
    return this.ensurePermission(
      this.screenRecordingPermission,
      'Screen recording',
      trigger,
      (): boolean => this.hasScreenRecordingPermission(),
      (context: common.UIAbilityContext): Promise<boolean> =>
        this.openPermissionOnSetting(context, SCREEN_RECORDING_PERMISSION, 'Screen recording'),
      false
    )
  }

  hasScreenRecordingPermission(): boolean {
    return this.hasPermission(SCREEN_RECORDING_PERMISSION, 'Screen recording')
  }

  private ensurePermission(state: PermissionRequestState, label: string, trigger: string,
    isGranted: PermissionGrantChecker, request: PermissionRequestFactory, requestOnce: boolean): Promise<boolean> {
    if (isGranted()) {
      RdpLogger.info(`${label} permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (state.request !== null) {
      return state.request
    }
    if (requestOnce && state.requested) {
      RdpLogger.warn(`${label} permission is not granted: ${trigger}`)
      return Promise.resolve(false)
    }

    const context = this.delegate.getContext()
    if (context === null) {
      RdpLogger.warn(`${label} permission request skipped: no UIAbilityContext`)
      return Promise.resolve(false)
    }

    state.requested = true
    state.request = request(context)
      .finally(() => {
        state.request = null
      })
    return state.request
  }

  private getSelfAccessTokenId(): number {
    const bundleInfo = bundleManager.getBundleInfoForSelfSync(
      bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION
    )
    return bundleInfo.appInfo.accessTokenId
  }

  private hasPermission(permission: Permissions, label: string): boolean {
    try {
      const atManager = abilityAccessCtrl.createAtManager()
      const status = atManager.checkAccessTokenSync(this.getSelfAccessTokenId(), permission)
      return status === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED
    } catch (error) {
      RdpLogger.error(`${label} permission check failed: ${JSON.stringify(error)}`)
      return false
    }
  }

  private openPermissionOnSetting(context: common.UIAbilityContext, permission: Permissions,
    label: string): Promise<boolean> {
    const atManager = abilityAccessCtrl.createAtManager()
    return atManager.openPermissionOnSetting(context, permission)
      .then((result: abilityAccessCtrl.SelectedResult) => {
        const granted = result === abilityAccessCtrl.SelectedResult.GRANTED ||
          this.hasPermission(permission, label)
        if (granted) {
          RdpLogger.info(`${label} setting result: granted=${granted} result=${result}`)
        } else {
          RdpLogger.warn(`${label} setting result: granted=${granted} result=${result}`)
        }
        return granted
      })
      .catch((error: Error) => {
        RdpLogger.error(`${label} setting request failed: ${JSON.stringify(error)}`)
        return false
      })
  }

  private openScreenRecordingPermissionOnSettingAfterDelay(context: common.UIAbilityContext): Promise<boolean> {
    return new Promise<boolean>((resolve) => {
      this.screenRecordingPermissionPromptDelayResolve = resolve
      this.screenRecordingPermissionPromptDelayTimer = setTimeout((): void => {
        const delayedResolve = this.screenRecordingPermissionPromptDelayResolve
        this.clearScreenRecordingPermissionPromptDelay()
        if (delayedResolve === null) {
          return
        }
        if (this.hasScreenRecordingPermission()) {
          delayedResolve(true)
          return
        }
        this.openPermissionOnSetting(context, SCREEN_RECORDING_PERMISSION, 'Screen recording')
          .then((granted: boolean) => {
            delayedResolve(granted)
          })
          .catch((error: Error) => {
            RdpLogger.error(`Screen recording setting request failed: ${JSON.stringify(error)}`)
            delayedResolve(false)
          })
      }, SCREEN_RECORDING_PERMISSION_PROMPT_DELAY_MS)
    })
  }

  private openPendingScreenRecordingPermissionPrompt(trigger: string): void {
    const delayedResolve = this.screenRecordingPermissionPromptDelayResolve
    if (delayedResolve === null) {
      return
    }
    const context = this.delegate.getContext()
    this.clearScreenRecordingPermissionPromptDelay()
    if (context === null) {
      RdpLogger.warn(`Screen recording permission request skipped: no UIAbilityContext`)
      delayedResolve(false)
      return
    }

    RdpLogger.info(`Screen recording permission requested immediately: ${trigger}`)
    this.openPermissionOnSetting(context, SCREEN_RECORDING_PERMISSION, 'Screen recording')
      .then((granted: boolean) => {
        delayedResolve(granted)
      })
      .catch((error: Error) => {
        RdpLogger.error(`Screen recording setting request failed: ${JSON.stringify(error)}`)
        delayedResolve(false)
      })
  }

  private clearScreenRecordingPermissionPromptDelay(): void {
    if (this.screenRecordingPermissionPromptDelayTimer !== null) {
      clearTimeout(this.screenRecordingPermissionPromptDelayTimer)
    }
    this.screenRecordingPermissionPromptDelayTimer = null
    this.screenRecordingPermissionPromptDelayResolve = null
  }

  private requestRuntimePermissions(context: common.UIAbilityContext, label: string, permissions: Permissions[],
    isGranted: PermissionGrantChecker): Promise<boolean> {
    const atManager = abilityAccessCtrl.createAtManager()
    return atManager.requestPermissionsFromUser(context, permissions)
      .then((result) => {
        let granted = false
        if (Array.isArray(result.authResults)) {
          for (let i = 0; i < result.authResults.length; i++) {
            if (result.authResults[i] === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED) {
              granted = true
              break
            }
          }
        }
        granted = granted || isGranted()
        if (granted) {
          RdpLogger.info(`${label} permission request result: granted=${granted}`)
        } else {
          RdpLogger.warn(`${label} permission request result: granted=${granted}`)
        }
        return granted
      })
      .catch((error: Error) => {
        RdpLogger.error(`${label} permission request failed: ${JSON.stringify(error)}`)
        return false
      })
  }

  private hasLocationPermission(): boolean {
    return this.hasPermission(APPROXIMATE_LOCATION_PERMISSION, 'Approximate location') ||
      this.hasPermission(LOCATION_PERMISSION, 'Location')
  }
}
