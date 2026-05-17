import { abilityAccessCtrl, bundleManager, common, Permissions } from '@kit.AbilityKit'
import { hilog } from '@kit.PerformanceAnalysisKit'
import {
  HILOG_DOMAIN,
  HILOG_TAG,
  MICROPHONE_PERMISSION,
  READ_PASTEBOARD_PERMISSION
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
  private microphonePermissionInFlight: boolean = false
  private microphonePermissionRequest: Promise<boolean> | null = null

  constructor(delegate: RdpPermissionDelegate) {
    this.delegate = delegate
  }

  ensurePasteboardReadPermission(trigger: string): void {
    if (this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard')) {
      this.appendPasteboardPermissionLog(`Pasteboard read permission granted: ${trigger}`)
      return
    }
    if (this.pasteboardPermissionRequested || this.pasteboardPermissionInFlight) {
      return
    }

    const context = this.delegate.getContext()
    if (context === null) {
      this.appendPasteboardPermissionLog('Pasteboard permission request skipped: no UIAbilityContext')
      return
    }

    this.pasteboardPermissionRequested = true
    this.pasteboardPermissionInFlight = true
    const atManager = abilityAccessCtrl.createAtManager()
    atManager.requestPermissionsFromUser(context, [READ_PASTEBOARD_PERMISSION])
      .then((result) => {
        const authResult = Array.isArray(result.authResults) && result.authResults.length > 0 ?
          result.authResults[0] : abilityAccessCtrl.GrantStatus.PERMISSION_DENIED
        const granted = authResult === abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED ||
          this.hasPermission(READ_PASTEBOARD_PERMISSION, 'Pasteboard')
        this.appendPasteboardPermissionLog(`Pasteboard read permission request result: granted=${granted} auth=${authResult}`)
      })
      .catch((error: Error) => {
        this.appendPasteboardPermissionLog(`Pasteboard read permission request failed: ${JSON.stringify(error)}`)
      })
      .finally(() => {
        this.pasteboardPermissionInFlight = false
      })
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

  private appendPasteboardPermissionLog(line: string): void {
    this.delegate.log(line)
    hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{public}s', line)
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
