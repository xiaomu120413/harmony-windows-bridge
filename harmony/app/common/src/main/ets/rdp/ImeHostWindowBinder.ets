import { window } from '@kit.ArkUI'
import { NativeCommandResult, NativeRdpGateway } from './NativeRdpGateway'
import { RdpLogger } from './RdpLogger'

export class ImeHostWindowBinder {
  bind(mainWindow: window.Window): void {
    try {
      const result: NativeCommandResult = NativeRdpGateway.bindImeHostWindow(mainWindow.getWindowProperties().id)
      if (!result.ok) {
        RdpLogger.warn(`IME host window rejected: ${result.message}`)
      }
    } catch (error) {
      RdpLogger.warn(`IME host window binding failed: ${JSON.stringify(error)}`)
    }
  }
}
