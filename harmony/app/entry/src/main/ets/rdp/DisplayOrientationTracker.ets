import { display, window } from '@kit.ArkUI'
import rdpNative, { NativeCommandResult } from 'libentry.so'
import { rdpOrientationForDisplayOrientation } from './DisplayOrientationPolicy'
import { RdpLogger } from './RdpLogger'

export class DisplayOrientationTracker {
  private mainWindow: window.Window | undefined = undefined
  private displayId: number = -1
  private readonly displayChangeCallback = (changedDisplayId: number): void => {
    if (changedDisplayId === this.displayId) {
      this.publishCurrentOrientation('display change')
    }
  }
  private readonly displayIdChangeCallback = (displayId: number): void => {
    this.displayId = displayId
    this.publishCurrentOrientation('window display change')
  }

  start(mainWindow: window.Window): void {
    this.stop()
    this.mainWindow = mainWindow
    this.displayId = this.resolveWindowDisplayId(mainWindow)
    display.on('change', this.displayChangeCallback)
    try {
      mainWindow.on('displayIdChange', this.displayIdChangeCallback)
    } catch (error) {
      RdpLogger.warn(`Window display listener registration failed: ${JSON.stringify(error)}`)
    }
    this.publishCurrentOrientation('window ready')
  }

  stop(): void {
    try {
      display.off('change', this.displayChangeCallback)
    } catch (error) {
      RdpLogger.warn(`Display orientation listener removal failed: ${JSON.stringify(error)}`)
    }
    if (this.mainWindow !== undefined) {
      try {
        this.mainWindow.off('displayIdChange', this.displayIdChangeCallback)
      } catch (error) {
        RdpLogger.warn(`Window display listener removal failed: ${JSON.stringify(error)}`)
      }
    }
    this.mainWindow = undefined
    this.displayId = -1
  }

  private resolveWindowDisplayId(mainWindow: window.Window): number {
    try {
      const displayId = mainWindow.getWindowProperties().displayId
      if (displayId !== undefined) {
        return displayId
      }
    } catch (error) {
      RdpLogger.warn(`Window display id unavailable: ${JSON.stringify(error)}`)
    }
    try {
      return display.getDefaultDisplaySync().id
    } catch (error) {
      RdpLogger.warn(`Default display id unavailable: ${JSON.stringify(error)}`)
      return -1
    }
  }

  private publishCurrentOrientation(source: string): void {
    try {
      const currentDisplay = this.displayId >= 0 ? display.getDisplayByIdSync(this.displayId) :
        display.getDefaultDisplaySync()
      const orientation = rdpOrientationForDisplayOrientation(currentDisplay.orientation)
      const result: NativeCommandResult = rdpNative.setDisplayOrientation(orientation)
      if (!result.ok) {
        RdpLogger.warn(`Display orientation rejected: source=${source} displayId=${currentDisplay.id} ` +
          `orientation=${orientation} ${result.message}`)
        return
      }
      RdpLogger.info(`Display orientation published: source=${source} displayId=${currentDisplay.id} ` +
        `displayOrientation=${currentDisplay.orientation} rdpOrientation=${orientation}`)
    } catch (error) {
      RdpLogger.warn(`Display orientation update failed: source=${source} error=${JSON.stringify(error)}`)
    }
  }
}
