import { deviceInfo } from '@kit.BasicServicesKit'

export enum RemoteControlServerCapability {
  AVAILABLE = 'available',
  UNAVAILABLE = 'unavailable'
}

export interface DeviceCapabilitySnapshot {
  readonly capabilitySnapshotId: string
  readonly sourceDeviceType: string
  readonly remoteControlServer: RemoteControlServerCapability
}

export function capabilitiesForDeviceType(deviceType: string): DeviceCapabilitySnapshot {
  const normalizedDeviceType = deviceType.trim().toLowerCase()
  const sourceDeviceType = normalizedDeviceType.length > 0 ? normalizedDeviceType : 'unknown'
  const remoteControlServer = sourceDeviceType === '2in1' ? RemoteControlServerCapability.AVAILABLE :
    RemoteControlServerCapability.UNAVAILABLE
  return {
    capabilitySnapshotId: `device:${sourceDeviceType}:remoteControlServer:${remoteControlServer}`,
    sourceDeviceType: sourceDeviceType,
    remoteControlServer: remoteControlServer
  }
}

export function currentDeviceCapabilities(): DeviceCapabilitySnapshot {
  return capabilitiesForDeviceType(deviceInfo.deviceType)
}

export function hasRemoteControlServer(snapshot: DeviceCapabilitySnapshot): boolean {
  return snapshot.remoteControlServer === RemoteControlServerCapability.AVAILABLE
}
