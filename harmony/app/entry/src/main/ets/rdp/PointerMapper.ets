import { PointerMapping } from './RdpTypes'

export interface PointerViewport {
  resolution: string
  surfacePixelWidth: number
  surfacePixelHeight: number
  surfaceLayoutWidth: number
  surfaceLayoutHeight: number
  surfaceViewportX: number
  surfaceViewportY: number
  surfaceViewportWidth: number
  surfaceViewportHeight: number
  desktopPixelWidth: number
  desktopPixelHeight: number
}

export class PointerMapper {
  static areaLengthToNumber(value: Length): number {
    if (typeof value === 'number') {
      return value
    }

    const text = `${value}`
    const unitIndex = text.search(/[a-zA-Z%]/)
    const numericText = unitIndex >= 0 ? text.substring(0, unitIndex) : text
    const numeric = Number(numericText)
    return numeric > 0 ? numeric : 0
  }

  static getPointerScaleX(viewport: PointerViewport): number {
    if (viewport.surfaceLayoutWidth > 0 && viewport.surfacePixelWidth > 0) {
      return viewport.surfacePixelWidth / viewport.surfaceLayoutWidth
    }
    return 1
  }

  static getPointerScaleY(viewport: PointerViewport): number {
    if (viewport.surfaceLayoutHeight > 0 && viewport.surfacePixelHeight > 0) {
      return viewport.surfacePixelHeight / viewport.surfaceLayoutHeight
    }
    return 1
  }

  static isPointInRemoteViewport(viewport: PointerViewport, x: number, y: number): boolean {
    const surfaceX = x * PointerMapper.getPointerScaleX(viewport)
    const surfaceY = y * PointerMapper.getPointerScaleY(viewport)
    const viewportX = PointerMapper.getRemoteViewportX(viewport)
    const viewportY = PointerMapper.getRemoteViewportY(viewport)
    const viewportWidth = PointerMapper.getRemoteViewportWidth(viewport)
    const viewportHeight = PointerMapper.getRemoteViewportHeight(viewport)
    if (viewportWidth <= 0 || viewportHeight <= 0) {
      return false
    }
    return surfaceX >= viewportX && surfaceX < viewportX + viewportWidth &&
      surfaceY >= viewportY && surfaceY < viewportY + viewportHeight
  }

  static mapToDesktop(viewport: PointerViewport, x: number, y: number, allowClamp: boolean): PointerMapping {
    const scaleX = PointerMapper.getPointerScaleX(viewport)
    const scaleY = PointerMapper.getPointerScaleY(viewport)
    const surfaceX = x * scaleX
    const surfaceY = y * scaleY
    const viewportX = PointerMapper.getRemoteViewportX(viewport)
    const viewportY = PointerMapper.getRemoteViewportY(viewport)
    const viewportWidth = PointerMapper.getRemoteViewportWidth(viewport)
    const viewportHeight = PointerMapper.getRemoteViewportHeight(viewport)
    const desktopWidth = PointerMapper.getRequestedDesktopWidth(viewport)
    const desktopHeight = PointerMapper.getRequestedDesktopHeight(viewport)
    const base: PointerMapping = {
      ok: false,
      x: 0,
      y: 0,
      surfaceX: surfaceX,
      surfaceY: surfaceY,
      scaleX: scaleX,
      scaleY: scaleY,
      viewportX: viewportX,
      viewportY: viewportY,
      viewportWidth: viewportWidth,
      viewportHeight: viewportHeight,
      desktopWidth: desktopWidth,
      desktopHeight: desktopHeight,
      reason: ''
    }
    if (viewportWidth <= 0 || viewportHeight <= 0 || desktopWidth <= 0 || desktopHeight <= 0) {
      base.reason = 'surface viewport or remote desktop size is not ready'
      return base
    }

    const localX = surfaceX - viewportX
    const localY = surfaceY - viewportY
    const inside = localX >= 0 && localX < viewportWidth && localY >= 0 && localY < viewportHeight
    if (!inside && !allowClamp) {
      base.reason = 'point is outside rendered remote viewport'
      return base
    }

    const clampedX = PointerMapper.clampInt(localX, 0, viewportWidth - 1)
    const clampedY = PointerMapper.clampInt(localY, 0, viewportHeight - 1)
    const mappedX = PointerMapper.clampInt(clampedX * desktopWidth / viewportWidth, 0, desktopWidth - 1)
    const mappedY = PointerMapper.clampInt(clampedY * desktopHeight / viewportHeight, 0, desktopHeight - 1)
    base.ok = true
    base.x = mappedX
    base.y = mappedY
    base.reason = 'ok'
    return base
  }

  private static parseResolutionPart(resolution: string, index: number): number {
    const parts = resolution.toLowerCase().split('x')
    if (parts.length !== 2) {
      return 0
    }

    const value = Number(parts[index])
    if (value > 0) {
      return value
    }
    return 0
  }

  private static getRequestedDesktopWidth(viewport: PointerViewport): number {
    if (viewport.desktopPixelWidth > 0) {
      return viewport.desktopPixelWidth
    }
    const parsedWidth = PointerMapper.parseResolutionPart(viewport.resolution, 0)
    if (parsedWidth > 0) {
      return parsedWidth
    }
    return PointerMapper.getRemoteViewportWidth(viewport)
  }

  private static getRequestedDesktopHeight(viewport: PointerViewport): number {
    if (viewport.desktopPixelHeight > 0) {
      return viewport.desktopPixelHeight
    }
    const parsedHeight = PointerMapper.parseResolutionPart(viewport.resolution, 1)
    if (parsedHeight > 0) {
      return parsedHeight
    }
    return PointerMapper.getRemoteViewportHeight(viewport)
  }

  private static clampInt(value: number, minValue: number, maxValue: number): number {
    return Math.max(minValue, Math.min(maxValue, Math.round(value)))
  }

  private static getRemoteViewportX(viewport: PointerViewport): number {
    return viewport.surfaceViewportWidth > 0 && viewport.surfaceViewportHeight > 0 ?
      viewport.surfaceViewportX : 0
  }

  private static getRemoteViewportY(viewport: PointerViewport): number {
    return viewport.surfaceViewportWidth > 0 && viewport.surfaceViewportHeight > 0 ?
      viewport.surfaceViewportY : 0
  }

  private static getRemoteViewportWidth(viewport: PointerViewport): number {
    return viewport.surfaceViewportWidth > 0 ? viewport.surfaceViewportWidth : viewport.surfacePixelWidth
  }

  private static getRemoteViewportHeight(viewport: PointerViewport): number {
    return viewport.surfaceViewportHeight > 0 ? viewport.surfaceViewportHeight : viewport.surfacePixelHeight
  }
}
