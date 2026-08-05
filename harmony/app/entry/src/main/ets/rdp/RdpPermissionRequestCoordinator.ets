import {
  NativeCommandResult,
  NativePermissionRequest,
  NativePermissionType,
  NativeRdpGateway
} from './NativeRdpGateway'
import { RdpLogger } from './RdpLogger'
import { RdpPermissionManager } from './RdpPermissions'

interface PermissionRoute {
  type: NativePermissionType
  label: string
  trigger: string
  ensurePermission: (trigger: string) => Promise<boolean>
}

export class RdpPermissionRequestCoordinator {
  private readonly permissionManager: RdpPermissionManager
  private readonly deferUiUpdate: (task: () => void) => void

  constructor(permissionManager: RdpPermissionManager, deferUiUpdate: (task: () => void) => void) {
    this.permissionManager = permissionManager
    this.deferUiUpdate = deferUiUpdate
  }

  register(): NativeCommandResult {
    const routes = this.routes()
    const result = NativeRdpGateway.onPermissionRequest((request: NativePermissionRequest): void => {
      this.deferUiUpdate((): void => {
        this.handleRequest(request, routes)
      })
    })
    if (!result.ok) {
      RdpLogger.error(`Native permission callback registration failed: ${result.message}`)
    }
    return result
  }

  private routes(): PermissionRoute[] {
    return [
      {
        type: 'microphone',
        label: 'Remote audio capture requested microphone permission',
        trigger: 'remote audio capture',
        ensurePermission: (trigger: string): Promise<boolean> =>
          this.permissionManager.ensureMicrophonePermission(trigger)
      },
      {
        type: 'camera',
        label: 'Remote camera redirection requested camera permission',
        trigger: 'remote camera redirection',
        ensurePermission: (trigger: string): Promise<boolean> =>
          this.permissionManager.ensureCameraPermission(trigger)
      },
      {
        type: 'clipboard',
        label: 'Clipboard sync requested pasteboard permission',
        trigger: 'clipboard read',
        ensurePermission: (trigger: string): Promise<boolean> =>
          this.permissionManager.ensurePasteboardReadPermission(trigger)
      },
      {
        type: 'location',
        label: 'RDP location channel requested location permission',
        trigger: 'remote location redirection',
        ensurePermission: (trigger: string): Promise<boolean> =>
          this.permissionManager.ensureLocationPermission(trigger)
      }
    ]
  }

  private findRoute(type: NativePermissionType, routes: PermissionRoute[]): PermissionRoute | undefined {
    for (const route of routes) {
      if (route.type === type) {
        return route
      }
    }
    return undefined
  }

  private handleRequest(request: NativePermissionRequest, routes: PermissionRoute[]): void {
    const route = this.findRoute(request.type, routes)
    if (route === undefined) {
      RdpLogger.error(`Unknown native permission request type: ${request.type}`)
      return
    }
    const parsedRequestId = Number(request.requestId)
    const requestId = isNaN(parsedRequestId) ? 0 : parsedRequestId
    RdpLogger.info(`${route.label}: requestId=${request.requestId}`)
    route.ensurePermission(route.trigger)
      .then((granted: boolean): void => {
        this.complete(request.type, requestId, granted)
      })
      .catch((error: Error): void => {
        RdpLogger.error(`${route.label} failed: ${JSON.stringify(error)}`)
        this.complete(request.type, requestId, false)
      })
  }

  private complete(type: NativePermissionType, requestId: number, granted: boolean): void {
    const result = NativeRdpGateway.completePermissionRequest({
      type: type,
      requestId: requestId,
      granted: granted
    })
    if (!granted || !result.ok) {
      RdpLogger.warn(`${result.message}: granted=${granted}`)
    }
  }
}
