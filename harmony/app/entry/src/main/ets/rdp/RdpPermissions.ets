import { abilityAccessCtrl, bundleManager, common, Permissions } from '@kit.AbilityKit'
import { hilog } from '@kit.PerformanceAnalysisKit'
import {
  ACCESS_USER_FULL_DISK_PERMISSION,
  APPROXIMATE_LOCATION_PERMISSION,
  HILOG_DOMAIN,
  HILOG_TAG,
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

export class RdpPermissionManager {
  private delegate: RdpPermissionDelegate
  private pasteboardPermissionRequested: boolean = false
  private pasteboardPermissionInFlight: boolean = false
  private pasteboardPermissionRequest: Promise<boolean> | null = null
  private microphonePermissionInFlight: boolean = false
  private microphonePermissionRequest: Promise<boolean> | null = null
  private locationPermissionInFlight: boolean = false
  private locationPermissionRequest: Promise<boolean> | null = null
  private fullDiskAccessPermissionRequested: boolean = false
  private fullDiskAccessPermissionRequest: Promise<boolean> | null = null
  private screenRecordingPermissionRequested: boolean = false
  private screenRecordingPermissionRequest: Promise<boolean> | null = null

  constructor(delegate: RdpPermissionDelegate) {
    this.delegate = delegate
  }

  ensurePasteboardReadPermission(trigger: string): Promise<boolean> {
    if (this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard')) {
      this.appendPasteboardPermissionLog(`Pasteboard read permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (this.pasteboardPermissionRequest !== null) {
      return this.pasteboardPermissionRequest
    }
    if (this.pasteboardPermissionRequested || this.pasteboardPermissionInFlight) {
      return Promise.resolve(false)
    }

    const context = this.delegate.getContext()
    if (context === null) {
      this.appendPasteboardPermissionLog('Pasteboard permission request skipped: no UIAbilityContext')
      return Promise.resolve(false)
    }

    this.pasteboardPermissionRequested = true
    this.pasteboardPermissionInFlight = true
    const atManager = abilityAccessCtrl.createAtManager()
    this.pasteboardPermissionRequest = atManager.requestPermissionsFromUser(context, [READ_PASTEBOARD_PERMISSION])
      .then((result) => {
        const authResult = Array.isArray(result.authResults) && result.authResults.length > 0 ?
          result.authResults[0] : abilityAccessCtrl.GrantStatus.PERMISSION_DENIED
        const granted = authResult === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED ||
          this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard')
        this.appendPasteboardPermissionLog(`Pasteboard read permission request result: granted=${granted} auth=${authResult}`)
        return granted
      })
      .catch((error: Error) => {
        this.appendPasteboardPermissionLog(`Pasteboard read permission request failed: ${JSON.stringify(error)}`)
        return false
      })
      .finally(() => {
        this.pasteboardPermissionInFlight = false
        this.pasteboardPermissionRequest = null
      })
    return this.pasteboardPermissionRequest
  }

  ensureMicrophonePermission(trigger: string): Promise<boolean> {
    if (this.hasPermission(MICROPHONE_PERMISSION, 'Microphone')) {
      this.delegate.log(`Microphone permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (this.microphonePermissionRequest !== null) {
      return this.microphonePermissionRequest
    }

    const context = this.delegate.getContext()
    if (context === null) {
      this.delegate.log('Microphone permission request skipped: no UIAbilityContext')
      return Promise.resolve(false)
    }

    this.microphonePermissionInFlight = true
    const atManager = abilityAccessCtrl.createAtManager()
    this.microphonePermissionRequest = atManager.requestPermissionsFromUser(context, [MICROPHONE_PERMISSION])
      .then((result) => {
        const authResult = Array.isArray(result.authResults) && result.authResults.length > 0 ?
          result.authResults[0] : abilityAccessCtrl.GrantStatus.PERMISSION_DENIED
        const granted = authResult === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED ||
          this.hasPermission(MICROPHONE_PERMISSION, 'Microphone')
        this.delegate.log(`Microphone permission request result: granted=${granted} auth=${authResult}`)
        return granted
      })
      .catch((error: Error) => {
        this.delegate.log(`Microphone permission request failed: ${JSON.stringify(error)}`)
        return false
      })
      .finally(() => {
        this.microphonePermissionInFlight = false
        this.microphonePermissionRequest = null
      })
    return this.microphonePermissionRequest
  }

  ensureLocationPermission(trigger: string): Promise<boolean> {
    if (this.hasLocationPermission()) {
      this.delegate.log(`Location permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (this.locationPermissionRequest !== null) {
      return this.locationPermissionRequest
    }

    const context = this.delegate.getContext()
    if (context === null) {
      this.delegate.log('Location permission request skipped: no UIAbilityContext')
      return Promise.resolve(false)
    }

    this.locationPermissionInFlight = true
    const permissions: Permissions[] = [APPROXIMATE_LOCATION_PERMISSION, LOCATION_PERMISSION]
    const atManager = abilityAccessCtrl.createAtManager()
    this.locationPermissionRequest = atManager.requestPermissionsFromUser(context, permissions)
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
        granted = granted || this.hasLocationPermission()
        this.delegate.log(`Location permission request result: granted=${granted}`)
        return granted
      })
      .catch((error: Error) => {
        this.delegate.log(`Location permission request failed: ${JSON.stringify(error)}`)
        return false
      })
      .finally(() => {
        this.locationPermissionInFlight = false
        this.locationPermissionRequest = null
      })
    return this.locationPermissionRequest
  }

  ensureFullDiskAccessPermission(trigger: string): Promise<boolean> {
    if (this.hasPermission(ACCESS_USER_FULL_DISK_PERMISSION, 'Full disk access')) {
      this.delegate.log(`Full disk access permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (this.fullDiskAccessPermissionRequest !== null) {
      return this.fullDiskAccessPermissionRequest
    }
    if (this.fullDiskAccessPermissionRequested) {
      this.delegate.log(`Full disk access permission is not granted: ${trigger}`)
      return Promise.resolve(false)
    }

    const context = this.delegate.getContext()
    if (context === null) {
      this.delegate.log('Full disk access permission request skipped: no UIAbilityContext')
      return Promise.resolve(false)
    }

    this.fullDiskAccessPermissionRequested = true
    this.fullDiskAccessPermissionRequest = this.openPermissionOnSetting(context,
      ACCESS_USER_FULL_DISK_PERMISSION, 'Full disk access')
      .finally(() => {
        this.fullDiskAccessPermissionRequest = null
      })
    return this.fullDiskAccessPermissionRequest
  }

  ensureScreenRecordingPermission(trigger: string): Promise<boolean> {
    if (this.hasPermission(SCREEN_RECORDING_PERMISSION, 'Screen recording')) {
      this.delegate.log(`Screen recording permission granted: ${trigger}`)
      return Promise.resolve(true)
    }
    if (this.screenRecordingPermissionRequest !== null) {
      return this.screenRecordingPermissionRequest
    }
    if (this.screenRecordingPermissionRequested) {
      this.delegate.log(`Screen recording permission is not granted: ${trigger}`)
      return Promise.resolve(false)
    }

    const context = this.delegate.getContext()
    if (context === null) {
      this.delegate.log('Screen recording permission request skipped: no UIAbilityContext')
      return Promise.resolve(false)
    }

    this.screenRecordingPermissionRequested = true
    this.screenRecordingPermissionRequest = this.openPermissionOnSetting(context,
      SCREEN_RECORDING_PERMISSION, 'Screen recording')
      .finally(() => {
        this.screenRecordingPermissionRequest = null
      })
    return this.screenRecordingPermissionRequest
  }

  private appendPasteboardPermissionLog(line: string): void {
    this.delegate.log(line)
    hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line)
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

  private hasLocationPermission(): boolean {
    return this.hasPermission(APPROXIMATE_LOCATION_PERMISSION, 'Approximate location') ||
      this.hasPermission(LOCATION_PERMISSION, 'Location')
  }
}
