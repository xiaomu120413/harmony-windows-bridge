import { Permissions } from '@kit.AbilityKit'

export const TOUCH_DRAG_THRESHOLD: number = 8
export const TOUCH_WHEEL_THRESHOLD: number = 24
export const LONG_PRESS_TIMEOUT_MS: number = 550
export const DOUBLE_TAP_TIMEOUT_MS: number = 320
export const DOUBLE_TAP_DISTANCE: number = 24
export const POINTER_FAILURE_LOG_INTERVAL_MS: number = 800
export const POINTER_STATE_REFRESH_INTERVAL_MS: number = 16
export const INPUT_EVENT_LOG_INTERVAL_MS: number = 650
export const SYNTHETIC_MOUSE_SUPPRESS_MS: number = 80
export const AXIS_EVENT_DEDUP_MS: number = 12

export const HILOG_DOMAIN: number = 0xF3D1
export const HILOG_TAG: string = 'FreeRDPUi'
export const REMOTE_SURFACE_FOCUS_ID: string = 'rdpRemoteSurface'

export const READ_PASTEBOARD_PERMISSION: Permissions = 'ohos.permission.READ_PASTEBOARD'
export const MICROPHONE_PERMISSION: Permissions = 'ohos.permission.MICROPHONE'
