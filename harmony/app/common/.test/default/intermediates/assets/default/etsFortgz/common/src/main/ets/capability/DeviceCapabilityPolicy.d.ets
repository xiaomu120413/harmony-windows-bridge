export declare enum RemoteControlServerCapability {
    AVAILABLE = "available",
    UNAVAILABLE = "unavailable"
}
export interface DeviceCapabilitySnapshot {
    readonly capabilitySnapshotId: string;
    readonly sourceDeviceType: string;
    readonly remoteControlServer: RemoteControlServerCapability;
}
export declare function capabilitiesForDeviceType(deviceType: string): DeviceCapabilitySnapshot;
export declare function currentDeviceCapabilities(): DeviceCapabilitySnapshot;
export declare function hasRemoteControlServer(snapshot: DeviceCapabilitySnapshot): boolean;
