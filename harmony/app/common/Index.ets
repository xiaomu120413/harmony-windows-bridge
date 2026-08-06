export function packagingProbe(deviceRole: string): string {
  return `PACKAGING_PROBE common.hsp role=${deviceRole}`
}

export { MuHubApp } from './src/main/ets/pages/Index'
export { SettingsTheme } from './src/main/ets/components/settings/SettingsUi'
export { RemoteFilesDirectory } from './src/main/ets/rdp/RemoteFilesDirectory'
export { ImeHostWindowBinder } from './src/main/ets/rdp/ImeHostWindowBinder'
export { RdpLogger } from './src/main/ets/rdp/RdpLogger'
