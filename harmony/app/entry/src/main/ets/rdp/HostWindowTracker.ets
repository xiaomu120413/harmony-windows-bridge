import { window } from '@kit.ArkUI'
import rdpNative, { NativeCommandResult } from 'libentry.so'
import { RdpLogger } from './RdpLogger'

export class HostWindowTracker {
  private mainWindow: window.Window | undefined = undefined
  private readonly displayIdChangeCallback = (_displayId: number): void => {
    this.publishHostWindow('display changed')
  }

  start(mainWindow: window.Window): void {
    this.stop()
    this.mainWindow = mainWindow
    try {
      mainWindow.on('displayIdChange', this.displayIdChangeCallback)
    } catch (error) {
      RdpLogger.warn(`Host window display listener registration failed: ${JSON.stringify(error)}`)
    }
    this.publishHostWindow('window ready')
  }

  stop(): void {
    if (this.mainWindow !== undefined) {
      try {
        this.mainWindow.off('displayIdChange', this.displayIdChangeCallback)
      } catch (error) {
        RdpLogger.warn(`Host window display listener removal failed: ${JSON.stringify(error)}`)
      }
    }
    this.mainWindow = undefined
  }

  private publishHostWindow(source: string): void {
    if (this.mainWindow === undefined) {
      return
    }
    try {
      const properties = this.mainWindow.getWindowProperties()
      const result: NativeCommandResult = rdpNative.configureHostWindow({
        windowId: properties.id,
        displayId: properties.displayId ?? 0
      })
      if (!result.ok) {
        RdpLogger.warn(`Host window rejected: source=${source} ${result.message}`)
      }
    } catch (error) {
      RdpLogger.warn(`Host window publish failed: source=${source} error=${JSON.stringify(error)}`)
    }
  }
}
