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
let bjccovmshb1icx = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/Index.ets", hash: "0c4876a7f96826c408ae8f9cfbb32a4636f47d33cff86e3198b3b82175ab90a4", lineCnt: 19, count: 0, projectPath: "", functions: { 0: { name: "packagingProbe", count: 0, regions: { 0: { startLoc: { line: 1, col: 1 }, endLoc: { line: 3, col: 2 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 2, col: 3 }, endLoc: { line: 3, col: 2 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 } }, exeLine: { 0: 1, 1: 2, 2: 5, 3: 6, 4: 7, 5: 8, 6: 9, 7: 10, 8: 11, 9: 12, 10: 13, 11: 14, 12: 15, 13: 16, 14: 17, 15: 18 } });
export function packagingProbe(deviceRole: string): string {
    bjccovmshb1icx.instrumentFunction(0);
    bjccovmshb1icx.instrumentRegion(0, 1);
    return `PACKAGING_PROBE common.hsp role=${deviceRole}`;
}
export { MuHubApp } from "@normalized:N&&&common/src/main/ets/pages/Index&";
export { SettingsTheme } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsUi&";
export { RemoteFilesDirectory } from "@normalized:N&&&common/src/main/ets/rdp/RemoteFilesDirectory&";
export { ImeHostWindowBinder } from "@normalized:N&&&common/src/main/ets/rdp/ImeHostWindowBinder&";
export { RdpLogger } from "@normalized:N&&&common/src/main/ets/rdp/RdpLogger&";
export { capabilitiesForDeviceType, RemoteControlServerCapability } from "@normalized:N&&&common/src/main/ets/capability/DeviceCapabilityPolicy&";
export { LayoutMode, layoutModeForWidthBreakpoint } from "@normalized:N&&&common/src/main/ets/adaptive/WindowLayoutPolicy&";
export { RdpConnectionValidator } from "@normalized:N&&&common/src/main/ets/rdp/RdpConnectionValidator&";
