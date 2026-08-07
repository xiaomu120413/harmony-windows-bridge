import { PrintExtensionAbility, print } from '@kit.BasicServicesKit'
import { Want } from '@kit.AbilityKit'
import {
  addMuHubVirtualPrinter,
  removeMuHubVirtualPrinter,
  RdpLogger,
  updateMuHubVirtualPrinter
} from 'common'

const TAG = 'MuHubPrintExtension'

export default class MuHubPrintExtension extends PrintExtensionAbility {
  onCreate(_want: Want): void {
    RdpLogger.info(`${TAG} created`)
  }

  onStartDiscoverPrinter(): void {
    addMuHubVirtualPrinter('extension-discover')
  }

  onStopDiscoverPrinter(): void {
    RdpLogger.info(`${TAG} stop discover printer`)
    removeMuHubVirtualPrinter('extension-stop-discover')
  }

  onConnectPrinter(printerId: number): void {
    RdpLogger.info(`${TAG} connect printer id=${printerId}`)
    updateMuHubVirtualPrinter('extension-connect')
  }

  onDisconnectPrinter(printerId: number): void {
    RdpLogger.info(`${TAG} disconnect printer id=${printerId}`)
    updateMuHubVirtualPrinter('extension-disconnect', print.PrinterStatus.PRINTER_UNAVAILABLE)
  }

  onDestroy(): void {
    RdpLogger.info(`${TAG} destroyed`)
  }
}
