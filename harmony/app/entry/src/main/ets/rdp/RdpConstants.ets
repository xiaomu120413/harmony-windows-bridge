import { Permissions } from '@kit.AbilityKit'

export const HILOG_DOMAIN: number = 0xF3D1
export const HILOG_TAG: string = 'FreeRDPUi'
export const REMOTE_SURFACE_FOCUS_ID: string = 'rdpRemoteSurface'

export const READ_PASTEBOARD_PERMISSION: Permissions = 'ohos.permission.READ_PASTEBOARD'
export const MICROPHONE_PERMISSION: Permissions = 'ohos.permission.MICROPHONE'

export type ConnectionState =
  'Idle' | 'Resolving' | 'TCP connected' | 'Negotiating' | 'Authenticating' |
    'Connected' | 'Bridge ready' | 'Disconnected' | 'Failed'