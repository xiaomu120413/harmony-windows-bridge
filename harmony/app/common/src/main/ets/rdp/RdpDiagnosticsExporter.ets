import { common } from '@kit.AbilityKit'
import { bundleManager } from '@kit.AbilityKit'
import { fileIo } from '@kit.CoreFileKit'
import { NativeRdpGateway } from './NativeRdpGateway'
import { RemoteFilesDirectory } from './RemoteFilesDirectory'
import { XrdpServerStatus } from './RemoteControlCoordinator'
import { RdpLogger } from './RdpLogger'
import { RdpPermissionManager } from './RdpPermissions'
import { RdpSessionDiagnosticSnapshot } from './RdpSessionLifecycleCoordinator'

export interface RdpDiagnosticsInput {
  hostLength: number
  usernameLength: number
  port: string
  remoteControlServerAvailable: boolean
  screenRecordingPermissionGranted: boolean
  inputInjectionPermissionGranted: boolean
  microphonePermissionGranted: boolean
  cameraPermissionGranted: boolean
  clipboardPermissionGranted: boolean
  locationPermissionGranted: boolean
  xrdpStatus: XrdpServerStatus
  session: RdpSessionDiagnosticSnapshot
}

export interface RdpDiagnosticsExportResult {
  ok: boolean
  path: string
  message: string
}

interface DiagnosticsApplication {
  bundleName: string
  versionName: string
  versionCode: number
}

interface DiagnosticsConnection {
  hostConfigured: boolean
  hostLength: number
  usernameConfigured: boolean
  usernameLength: number
  port: string
  stage: string
  endpointProbeState: string
  endpointProbeElapsedMs: number
}

interface DiagnosticsPermissions {
  screenRecording: boolean
  inputInjection: boolean
  microphone: boolean
  camera: boolean
  clipboard: boolean
  location: boolean
}

interface DiagnosticsFreerdp {
  nativeSummary: string
  lastErrorCategory: string
}

interface DiagnosticsCoverage {
  connectionStage: string
  endpointProbe: string
  renderAndGraphics: string
  audio: string
  resize: string
  clipboardCapability: string
  clipboardRuntimeCounters: string
  cameraCapability: string
  cameraPacketCounters: string
  xrdpLifecycle: string
  passiveScreenCaptureCounters: string
  passiveInputInjectionCounters: string
}

interface DiagnosticsStorage {
  sharedDirectoryPrepared: boolean
}

interface DiagnosticsXrdp {
  available: boolean
  running: boolean
  state: string
  port: number
  pid: number
  lastExitCode: number
  message: string
}

interface RdpDiagnosticsReport {
  schemaVersion: number
  exportedAt: string
  application: DiagnosticsApplication
  connection: DiagnosticsConnection
  permissions: DiagnosticsPermissions
  freerdp: DiagnosticsFreerdp
  xrdp: DiagnosticsXrdp
  coverage: DiagnosticsCoverage
  storage: DiagnosticsStorage
}

export class RdpDiagnosticsExporter {
  static capture(context: common.UIAbilityContext | null, hostLength: number, usernameLength: number,
    port: string, remoteControlServerAvailable: boolean, permissionManager: RdpPermissionManager,
    xrdpStatus: XrdpServerStatus, session: RdpSessionDiagnosticSnapshot): Promise<RdpDiagnosticsExportResult> {
    return RdpDiagnosticsExporter.export(context, {
      hostLength: hostLength,
      usernameLength: usernameLength,
      port: port,
      remoteControlServerAvailable: remoteControlServerAvailable,
      screenRecordingPermissionGranted: permissionManager.hasScreenRecordingPermission(),
      inputInjectionPermissionGranted: permissionManager.hasControlDevicePermission(),
      microphonePermissionGranted: permissionManager.hasMicrophonePermission(),
      cameraPermissionGranted: permissionManager.hasCameraPermission(),
      clipboardPermissionGranted: permissionManager.hasPasteboardReadPermission(),
      locationPermissionGranted: permissionManager.hasAnyLocationPermission(),
      xrdpStatus: xrdpStatus,
      session: session
    })
  }

  static async export(context: common.UIAbilityContext | null,
    input: RdpDiagnosticsInput): Promise<RdpDiagnosticsExportResult> {
    if (context === null) {
      return RdpDiagnosticsExporter.failed('无法获取应用上下文')
    }
    if (RemoteFilesDirectory.sharedDirectoryPath().length === 0 &&
      !await RemoteFilesDirectory.prepareSharedDirectory(context)) {
      return RdpDiagnosticsExporter.failed('未选择诊断导出目录')
    }
    const directory = RemoteFilesDirectory.sharedDirectoryPath()
    if (directory.length === 0) {
      return RdpDiagnosticsExporter.failed('诊断导出目录不可用')
    }

    try {
      const bundleInfo = bundleManager.getBundleInfoForSelfSync(bundleManager.BundleFlag.GET_BUNDLE_INFO_DEFAULT)
      const exportedAt = new Date().toISOString()
      const application: DiagnosticsApplication = {
        bundleName: bundleInfo.name,
        versionName: bundleInfo.versionName,
        versionCode: bundleInfo.versionCode
      }
      const connection: DiagnosticsConnection = {
        hostConfigured: input.hostLength > 0,
        hostLength: input.hostLength,
        usernameConfigured: input.usernameLength > 0,
        usernameLength: input.usernameLength,
        port: input.port,
        stage: input.session.stage,
        endpointProbeState: input.session.endpointProbeState,
        endpointProbeElapsedMs: input.session.endpointProbeElapsedMs
      }
      const permissions: DiagnosticsPermissions = {
        screenRecording: input.screenRecordingPermissionGranted,
        inputInjection: input.inputInjectionPermissionGranted,
        microphone: input.microphonePermissionGranted,
        camera: input.cameraPermissionGranted,
        clipboard: input.clipboardPermissionGranted,
        location: input.locationPermissionGranted
      }
      const nativeSummary = NativeRdpGateway.getDiagnostics()
      const freerdp: DiagnosticsFreerdp = {
        nativeSummary: nativeSummary,
        lastErrorCategory: input.session.lastErrorCategory
      }
      const xrdp: DiagnosticsXrdp = {
        available: input.remoteControlServerAvailable,
        running: input.xrdpStatus.running,
        state: input.xrdpStatus.state,
        port: input.xrdpStatus.port,
        pid: input.xrdpStatus.pid || 0,
        lastExitCode: input.xrdpStatus.lastExitCode || 0,
        message: input.xrdpStatus.message
      }
      const report: RdpDiagnosticsReport = {
        schemaVersion: 2,
        exportedAt: exportedAt,
        application: application,
        connection: connection,
        permissions: permissions,
        freerdp: freerdp,
        xrdp: xrdp,
        coverage: {
          connectionStage: 'available',
          endpointProbe: 'available',
          renderAndGraphics: 'available',
          audio: 'available',
          resize: 'available',
          clipboardCapability: nativeSummary.includes('channel_clipboard=available') ? 'available' : 'unavailable',
          clipboardRuntimeCounters: nativeSummary.includes('OHOS clipboard stats: unavailable') ?
            'unavailable' : 'available',
          cameraCapability: nativeSummary.includes('channel_rdpecam=available') ? 'available' : 'unavailable',
          cameraPacketCounters: 'unavailable',
          xrdpLifecycle: input.remoteControlServerAvailable ? 'available' : 'unavailable',
          passiveScreenCaptureCounters: 'unavailable',
          passiveInputInjectionCounters: 'unavailable'
        },
        storage: { sharedDirectoryPrepared: directory.length > 0 }
      }
      const fileName = `muhub-diagnostics-${exportedAt.replace(/[:.]/g, '-')}.json`
      const path = `${directory}/${fileName}`
      const file = fileIo.openSync(path,
        fileIo.OpenMode.CREATE | fileIo.OpenMode.WRITE_ONLY | fileIo.OpenMode.TRUNC)
      try {
        fileIo.writeSync(file.fd, JSON.stringify(report, null, 2))
      } finally {
        fileIo.closeSync(file.fd)
      }
      RdpLogger.info(`RDP diagnostics exported: ${fileName}`)
      return {
        ok: true,
        path: path,
        message: `诊断已导出：${fileName}`
      }
    } catch (error) {
      RdpLogger.error(`RDP diagnostics export failed: ${JSON.stringify(error)}`)
      return RdpDiagnosticsExporter.failed('诊断导出失败，请检查目录权限')
    }
  }

  private static failed(message: string): RdpDiagnosticsExportResult {
    return { ok: false, path: '', message: message }
  }
}
