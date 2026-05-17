export type ConnectionState =
  'Idle' | 'Resolving' | 'TCP connected' | 'Negotiating' | 'Authenticating' |
    'Connected' | 'Bridge ready' | 'Disconnected' | 'Failed'

export interface NativeProbeResult {
  surfaceReady: boolean
  surfaceWidth: number
  surfaceHeight: number
  surfaceViewportX: number
  surfaceViewportY: number
  surfaceViewportWidth: number
  surfaceViewportHeight: number
  desktopWidth: number
  desktopHeight: number
  logs: string[]
}

export interface PointerMapping {
  ok: boolean
  x: number
  y: number
  surfaceX: number
  surfaceY: number
  scaleX: number
  scaleY: number
  viewportX: number
  viewportY: number
  viewportWidth: number
  viewportHeight: number
  desktopWidth: number
  desktopHeight: number
  reason: string
}

export interface NativeCommandResult {
  ok: boolean
  state: string
  message: string
  logs: string[]
}
