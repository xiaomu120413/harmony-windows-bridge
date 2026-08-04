import { describe, expect, it } from '@ohos/hypium'
import {
  LayoutMode,
  layoutModeForWidthBreakpoint
} from '../main/ets/adaptive/WindowLayoutPolicy'

export default function windowLayoutPolicyTest(): void {
  describe('WindowLayoutPolicy', (): void => {
    it('mapsWidthXsToCompact', 0, (): void => {
      expect(layoutModeForWidthBreakpoint(WidthBreakpoint.WIDTH_XS)).assertEqual(LayoutMode.COMPACT)
    })

    it('mapsWidthSmToCompact', 0, (): void => {
      expect(layoutModeForWidthBreakpoint(WidthBreakpoint.WIDTH_SM)).assertEqual(LayoutMode.COMPACT)
    })

    it('mapsWidthMdToCompact', 0, (): void => {
      expect(layoutModeForWidthBreakpoint(WidthBreakpoint.WIDTH_MD)).assertEqual(LayoutMode.COMPACT)
    })

    it('mapsWidthLgToExpanded', 0, (): void => {
      expect(layoutModeForWidthBreakpoint(WidthBreakpoint.WIDTH_LG)).assertEqual(LayoutMode.EXPANDED)
    })

    it('mapsWidthXlToExpanded', 0, (): void => {
      expect(layoutModeForWidthBreakpoint(WidthBreakpoint.WIDTH_XL)).assertEqual(LayoutMode.EXPANDED)
    })
  })
}
