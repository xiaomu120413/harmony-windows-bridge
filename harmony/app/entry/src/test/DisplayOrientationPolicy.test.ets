import { display } from '@kit.ArkUI'
import { describe, expect, it } from '@ohos/hypium'
import {
  RdpDisplayOrientation,
  rdpOrientationForDisplayOrientation
} from '../main/ets/rdp/DisplayOrientationPolicy'

export default function displayOrientationPolicyTest(): void {
  describe('DisplayOrientationPolicy', (): void => {
    it('mapsLandscapeToZero', 0, (): void => {
      expect(rdpOrientationForDisplayOrientation(display.Orientation.LANDSCAPE))
        .assertEqual(RdpDisplayOrientation.LANDSCAPE)
    })

    it('mapsPortraitToNinety', 0, (): void => {
      expect(rdpOrientationForDisplayOrientation(display.Orientation.PORTRAIT))
        .assertEqual(RdpDisplayOrientation.PORTRAIT)
    })

    it('mapsLandscapeInvertedToOneEighty', 0, (): void => {
      expect(rdpOrientationForDisplayOrientation(display.Orientation.LANDSCAPE_INVERTED))
        .assertEqual(RdpDisplayOrientation.LANDSCAPE_INVERTED)
    })

    it('mapsPortraitInvertedToTwoSeventy', 0, (): void => {
      expect(rdpOrientationForDisplayOrientation(display.Orientation.PORTRAIT_INVERTED))
        .assertEqual(RdpDisplayOrientation.PORTRAIT_INVERTED)
    })
  })
}
