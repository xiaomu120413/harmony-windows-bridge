import { window } from '@kit.ArkUI'
import rdpNative, { NativeCommandResult } from 'libentry.so'
import { RdpLogger } from './RdpLogger'

export class ImeHostWindowBinder {
  bind(mainWindow: window.Window): void {
    try {
      const result: NativeCommandResult = rdpNative.bindImeHostWindow(mainWindow.getWindowProperties().id)
      if (!result.ok) {
        RdpLogger.warn(`IME host window rejected: ${result.message}`)
      }
    } catch (error) {
      RdpLogger.warn(`IME host window binding failed: ${JSON.stringify(error)}`)
    }
  }
}
