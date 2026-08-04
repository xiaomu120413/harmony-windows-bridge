import { display } from '@kit.ArkUI'

export enum RdpDisplayOrientation {
  LANDSCAPE = 0,
  PORTRAIT = 90,
  LANDSCAPE_INVERTED = 180,
  PORTRAIT_INVERTED = 270
}

export function rdpOrientationForDisplayOrientation(orientation: display.Orientation): RdpDisplayOrientation {
  switch (orientation) {
    case display.Orientation.PORTRAIT:
      return RdpDisplayOrientation.PORTRAIT
    case display.Orientation.LANDSCAPE:
      return RdpDisplayOrientation.LANDSCAPE
    case display.Orientation.PORTRAIT_INVERTED:
      return RdpDisplayOrientation.PORTRAIT_INVERTED
    case display.Orientation.LANDSCAPE_INVERTED:
      return RdpDisplayOrientation.LANDSCAPE_INVERTED
    default:
      return RdpDisplayOrientation.LANDSCAPE
  }
}
