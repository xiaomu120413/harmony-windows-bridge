import { abilityAccessCtrl, bundleManager, common, Permissions } from '@kit.AbilityKit'
import {
  APPROXIMATE_LOCATION_PERMISSION,
  LOCATION_PERMISSION,
  MICROPHONE_PERMISSION,
  READ_PASTEBOARD_PERMISSION,
  SCREEN_RECORDING_PERMISSION
} from './RdpConstants'

export interface RdpPermissionDelegate {
  getContext(): common.UIAbilityContext | null
  log(line: string): void
}

export class RdpPermissionCallbackDelegate implements RdpPermissionDelegate {
  private readonly getContextCallback: () => common.UIAbilityContext | null
  private readonly logCallback: (line: string) => void

  constructor(getContext: () => common.UIAbilityContext | null, log: (line: string) => void) {
    this.getContextCallback = getContext
    this.logCallback = log
  }

  getContext(): common.UIAbilityContext | null {
    return this.getContextCallback()
  }

  log(line: string): void {
    this.logCallback(line)
  }
}

interface PermissionRequestState {
  request: Promise<boolean> | null
  requested: boolean
}

type PermissionGrantChecker = () => boolean
type PermissionRequestFactory = (context: common.UIAbilityContext) => Promise<boolean>

export class RdpPermissionManager {
  private delegate: RdpPermissionDelegate
  private pasteboardPermission: PermissionRequestState = { request: null, requested: false }
  private microphonePermission: PermissionRequestState = { request: null, requested: false }
  private locationPermission: PermissionRequestState = { request: null, requested: false }
  private screenRecordingPermission: PermissionRequestState = { request: null, requested: false }

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
      (): boolean => this.hasPermission(SCREEN_RECORDING_PERMISSION, 'Screen recording'),
      (context: common.UIAbilityContext): Promise<boolean> =>
        this.openPermissionOnSetting(context, SCREEN_RECORDING_PERMISSION, 'Screen recording'),
      true
    )
  }

  private ensurePermission(state: PermissionRequestState, label: string, trigger: string,
    isGranted: PermissionGrantChecker, request: PermissionRequestFactory, requestOnce: boolean): Promise<boolean> {
    if (isGranted()) {
      this.delegate.log(`${label} permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (state.request !== null) {
      return state.request
    }
    if (requestOnce && state.requested) {
      this.delegate.log(`${label} permission is not granted: ${trigger}`)
      return Promise.resolve(false)
    }

    const context = this.delegate.getContext()
    if (context === null) {
      this.delegate.log(`${label} permission request skipped: no UIAbilityContext`)
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
      this.delegate.log(`${label} permission check failed: ${JSON.stringify(error)}`)
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
        this.delegate.log(`${label} setting result: granted=${granted} result=${result}`)
        return granted
      })
      .catch((error: Error) => {
        this.delegate.log(`${label} setting request failed: ${JSON.stringify(error)}`)
        return false
      })
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
        this.delegate.log(`${label} permission request result: granted=${granted}`)
        return granted
      })
      .catch((error: Error) => {
        this.delegate.log(`${label} permission request failed: ${JSON.stringify(error)}`)
        return false
      })
  }

  private hasLocationPermission(): boolean {
    return this.hasPermission(APPROXIMATE_LOCATION_PERMISSION, 'Approximate location') ||
      this.hasPermission(LOCATION_PERMISSION, 'Location')
  }
}
