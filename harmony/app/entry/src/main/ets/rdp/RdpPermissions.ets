import { abilityAccessCtrl, bundleManager, common, Permissions } from '@kit.AbilityKit'
import { hilog } from '@kit.PerformanceAnalysisKit'
import {
  ACCESS_USER_FULL_DISK_PERMISSION,
  HILOG_DOMAIN,
  HILOG_TAG,
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
  private microphonePermissionRequest: Promise<boolean> | null = null
  private fullDiskAccessPermissionRequested: boolean = false
  private fullDiskAccessPermissionRequest: Promise<boolean> | null = null
  private screenRecordingPermissionRequested: boolean = false
  private screenRecordingPermissionRequest: Promise<boolean> | null = null

  constructor(delegate: RdpPermissionDelegate) {
    this.delegate = delegate
  }

  ensurePasteboardReadPermission(trigger: string): void {
    if (this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard')) {
      this.appendPasteboardPermissionLog(`Pasteboard read permission granted: ${trigger}`)
      return
    }
    if (this.pasteboardPermissionRequested) {
      return
    }

    const context = this.delegate.getContext()
    if (context === null) {
      this.appendPasteboardPermissionLog('Pasteboard permission request skipped: no UIAbilityContext')
      return
    }

    this.pasteboardPermissionRequested = true
    this.requestPermissionFromUser(context, READ_PASTEBOARD_PERMISSION, 'Pasteboard read',
      (line: string): void => this.appendPasteboardPermissionLog(line))
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

    this.microphonePermissionRequest = this.requestPermissionFromUser(context, MICROPHONE_PERMISSION, 'Microphone',
      (line: string): void => this.delegate.log(line))
      .finally(() => {
        this.microphonePermissionRequest = null
      })
    return this.microphonePermissionRequest
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
    hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{public}s', line)
  }

  private requestPermissionFromUser(context: common.UIAbilityContext, permission: Permissions, label: string,
    log: (line: string) => void): Promise<boolean> {
    const atManager = abilityAccessCtrl.createAtManager()
    return atManager.requestPermissionsFromUser(context, [permission])
      .then((result) => {
        const authResult = Array.isArray(result.authResults) && result.authResults.length > 0 ?
          result.authResults[0] : abilityAccessCtrl.GrantStatus.PERMISSION_DENIED
        const granted = authResult === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED ||
          this.hasPermission(permission, label)
        log(`${label} permission request result: granted=${granted} auth=${authResult}`)
        return granted
      })
      .catch((error: Error) => {
        log(`${label} permission request failed: ${JSON.stringify(error)}`)
        return false
      })
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
}
