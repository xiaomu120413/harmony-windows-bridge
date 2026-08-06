import deviceInfo from "@ohos:deviceInfo";
class BjcCov {
    coverage: {
        [key: string]: string | number;
    };
    constructor(covData: object) {
        const gcv = "__coverage__";
        let coverage = globalThis[gcv] || (globalThis[gcv] = {});
        if (!coverage[covData.path] && true) {
            coverage[covData.path] = covData;
        }
        this.coverage = coverage[covData.path];
    }
    instrumentFunction(func: number) {
        this.coverage.functions[func].count++;
        this.coverage.functions[func].regions[0].count++;
    }
    instrumentRegion(func: number, region: number) {
        this.coverage.functions[func].regions[region].count++;
    }
    instrumentReturn(func: number, retIdx: number) {
        this.coverage.functions[func].returnes[retIdx].count++;
    }
    instrumentBranch(func: number, branch: number, trueOrFalse: boolean) {
        if (trueOrFalse) {
            this.coverage.functions[func].branches[branch].trueCount++;
            for (let r of Object.values(this.coverage.functions[func].branches[branch].group)) {
                if (r !== branch) {
                    this.coverage.functions[func].branches[r as number].falseCount++;
                }
            }
        }
        else {
            this.coverage.functions[func].branches[branch].falseCount++;
        }
    }
}
let bjccovmshb1hzu = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/capability/DeviceCapabilityPolicy.ets", hash: "575de2f07e00be5087f059b946d63800fd7788144cc54ee714c08d97a8c866a7", lineCnt: 33, count: 0, projectPath: "", functions: { 0: { name: "capabilitiesForDeviceType", count: 0, regions: { 0: { startLoc: { line: 14, col: 1 }, endLoc: { line: 24, col: 2 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 15, col: 3 }, endLoc: { line: 24, col: 2 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 16, col: 28 }, endLoc: { line: 16, col: 94 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 17, col: 31 }, endLoc: { line: 18, col: 46 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 }, 1: { name: "currentDeviceCapabilities", count: 0, regions: { 0: { startLoc: { line: 26, col: 1 }, endLoc: { line: 28, col: 2 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 27, col: 3 }, endLoc: { line: 28, col: 2 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "hasRemoteControlServer", count: 0, regions: { 0: { startLoc: { line: 30, col: 1 }, endLoc: { line: 32, col: 2 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 31, col: 3 }, endLoc: { line: 32, col: 2 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 } }, exeLine: { 0: 1, 1: 3, 2: 4, 3: 5, 4: 8, 5: 9, 6: 10, 7: 11, 8: 14, 9: 15, 10: 16, 11: 17, 12: 18, 13: 19, 14: 20, 15: 21, 16: 22, 17: 26, 18: 27, 19: 30, 20: 31 } });
export enum RemoteControlServerCapability {
    AVAILABLE = "available",
    UNAVAILABLE = "unavailable"
}
export interface DeviceCapabilitySnapshot {
    readonly capabilitySnapshotId: string;
    readonly sourceDeviceType: string;
    readonly remoteControlServer: RemoteControlServerCapability;
}
export function capabilitiesForDeviceType(deviceType: string): DeviceCapabilitySnapshot {
    bjccovmshb1hzu.instrumentFunction(0);
    bjccovmshb1hzu.instrumentRegion(0, 1);
    const normalizedDeviceType = deviceType.trim().toLowerCase();
    const sourceDeviceType = normalizedDeviceType.length > 0 ? (bjccovmshb1hzu.instrumentBranch(0, 0, true), normalizedDeviceType) : (bjccovmshb1hzu.instrumentBranch(0, 0, false), 'unknown');
    const remoteControlServer = sourceDeviceType === '2in1' ? (bjccovmshb1hzu.instrumentBranch(0, 1, true), RemoteControlServerCapability.AVAILABLE) : (bjccovmshb1hzu.instrumentBranch(0, 1, false), RemoteControlServerCapability.UNAVAILABLE);
    return {
        capabilitySnapshotId: `device:${sourceDeviceType}:remoteControlServer:${remoteControlServer}`,
        sourceDeviceType: sourceDeviceType,
        remoteControlServer: remoteControlServer
    };
}
export function currentDeviceCapabilities(): DeviceCapabilitySnapshot {
    bjccovmshb1hzu.instrumentFunction(1);
    bjccovmshb1hzu.instrumentRegion(1, 1);
    return capabilitiesForDeviceType(deviceInfo.deviceType);
}
export function hasRemoteControlServer(snapshot: DeviceCapabilitySnapshot): boolean {
    bjccovmshb1hzu.instrumentFunction(2);
    bjccovmshb1hzu.instrumentRegion(2, 1);
    return snapshot.remoteControlServer === RemoteControlServerCapability.AVAILABLE;
}
