export enum LayoutMode {
  COMPACT = 'compact',
  EXPANDED = 'expanded'
}

export function layoutModeForWidthBreakpoint(widthBreakpoint: WidthBreakpoint): LayoutMode {
  switch (widthBreakpoint) {
    case WidthBreakpoint.WIDTH_LG:
    case WidthBreakpoint.WIDTH_XL:
      return LayoutMode.EXPANDED
    case WidthBreakpoint.WIDTH_XS:
    case WidthBreakpoint.WIDTH_SM:
    case WidthBreakpoint.WIDTH_MD:
    default:
      return LayoutMode.COMPACT
  }
}
