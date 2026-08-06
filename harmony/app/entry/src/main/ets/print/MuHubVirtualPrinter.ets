import { print } from '@kit.BasicServicesKit'
import { RdpLogger } from 'common'

const TAG = 'MuHubVirtualPrinter'

export const MUHUB_PRINT_EXTENSION_BUNDLE = 'com.muhub.desktop'
export const MUHUB_VIRTUAL_PRINTER_ID = 'muhub.windows.rdp.printer'
export const MUHUB_PRINTER_DEVICE_URI = 'rdpdr://muhub/windows-printer'

export const MUHUB_A4_PAGE_SIZE: print.PrintPageSize = {
  id: 'ISO_A4',
  name: 'iso_a4_210x297mm',
  width: 8268,
  height: 11692
}

export function buildMuHubPrinterInformation(status: print.PrinterStatus): print.PrinterInformation {
  return {
    printerId: MUHUB_VIRTUAL_PRINTER_ID,
    printerName: 'MuHub Windows Printer',
    printerStatus: status,
    description: 'Windows printer redirected by the active RDP session',
    uri: MUHUB_PRINTER_DEVICE_URI,
    printerMake: 'MuHub',
    alias: 'MuHub Windows Printer',
    capability: {
      supportedPageSizes: [
        MUHUB_A4_PAGE_SIZE
      ],
      supportedColorModes: [
        print.PrintColorMode.COLOR_MODE_COLOR,
        print.PrintColorMode.COLOR_MODE_MONOCHROME
      ],
      supportedDuplexModes: [
        print.PrintDuplexMode.DUPLEX_MODE_NONE,
        print.PrintDuplexMode.DUPLEX_MODE_LONG_EDGE
      ],
      supportedMediaTypes: [
        'stationery'
      ],
      supportedQualities: [
        print.PrintQuality.QUALITY_NORMAL
      ],
      supportedOrientations: [
        print.PrintOrientationMode.ORIENTATION_MODE_PORTRAIT,
        print.PrintOrientationMode.ORIENTATION_MODE_LANDSCAPE
      ]
    },
    preferences: {
      defaultPageSizeId: 'ISO_A4',
      defaultDuplexMode: print.PrintDuplexMode.DUPLEX_MODE_NONE,
      defaultPrintQuality: print.PrintQuality.QUALITY_NORMAL,
      defaultMediaType: 'stationery',
      defaultOrientation: print.PrintOrientationMode.ORIENTATION_MODE_PORTRAIT
    }
  }
}

export function addMuHubVirtualPrinter(reason: string,
  status: print.PrinterStatus = print.PrinterStatus.PRINTER_IDLE): void {
  print.addPrinterToDiscovery(buildMuHubPrinterInformation(status))
    .then(() => {
      RdpLogger.info(`${TAG} registered virtual printer reason=${reason}`)
    })
    .catch((error: Error) => {
      RdpLogger.error(`${TAG} register virtual printer failed reason=${reason}: ${JSON.stringify(error)}`)
    })
}

export function updateMuHubVirtualPrinter(reason: string,
  status: print.PrinterStatus = print.PrinterStatus.PRINTER_IDLE): void {
  print.updatePrinterInDiscovery(buildMuHubPrinterInformation(status))
    .then(() => RdpLogger.info(`${TAG} updated virtual printer reason=${reason}`))
    .catch((error: Error) => {
      RdpLogger.error(`${TAG} update virtual printer failed reason=${reason}: ${JSON.stringify(error)}`)
      addMuHubVirtualPrinter(`${reason}-fallback`, status)
    })
}

export function removeMuHubVirtualPrinter(reason: string): void {
  print.removePrinterFromDiscovery(MUHUB_VIRTUAL_PRINTER_ID)
    .then(() => RdpLogger.info(`${TAG} removed virtual printer reason=${reason}`))
    .catch((error: Error) => {
      RdpLogger.warn(`${TAG} remove virtual printer failed reason=${reason}: ${JSON.stringify(error)}`)
    })
}

export function startMuHubPrintDiscovery(reason: string): void {
  print.startDiscoverPrinter([MUHUB_PRINT_EXTENSION_BUNDLE])
    .then(() => {
      RdpLogger.info(`${TAG} started print discovery reason=${reason}`)
    })
    .catch((error: Error) => {
      RdpLogger.error(`${TAG} start print discovery failed reason=${reason}: ${JSON.stringify(error)}`)
    })
}
