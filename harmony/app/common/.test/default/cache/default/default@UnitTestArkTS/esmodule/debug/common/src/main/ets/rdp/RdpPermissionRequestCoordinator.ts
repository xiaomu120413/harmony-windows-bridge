import { NativeRdpGateway } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
import type { NativeCommandResult, NativePermissionRequest, NativePermissionType } from "@normalized:N&&&common/src/main/ets/rdp/NativeRdpGateway&";
import { RdpLogger } from "@normalized:N&&&common/src/main/ets/rdp/RdpLogger&";
import type { RdpPermissionManager } from './RdpPermissions';
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
let bjccovmshb1i16 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/RdpPermissionRequestCoordinator.ets", hash: "f556c2b755985a7b942d63db9399fd9d2236dce436eff31f76fe84b9a11afd44", lineCnt: 111, count: 0, projectPath: "", functions: { 0: { name: "anonymous_0", count: 0, regions: { 0: { startLoc: { line: 14, col: 21 }, endLoc: { line: 14, col: 58 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "anonymous_1", count: 0, regions: { 0: { startLoc: { line: 19, col: 35 }, endLoc: { line: 19, col: 61 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "anonymous_2", count: 0, regions: { 0: { startLoc: { line: 19, col: 42 }, endLoc: { line: 19, col: 52 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "RdpPermissionRequestCoordinator.constructor", count: 0, regions: { 0: { startLoc: { line: 21, col: 3 }, endLoc: { line: 24, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 22, col: 5 }, endLoc: { line: 24, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "anonymous_3", count: 0, regions: { 0: { startLoc: { line: 21, col: 71 }, endLoc: { line: 21, col: 97 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "anonymous_4", count: 0, regions: { 0: { startLoc: { line: 21, col: 78 }, endLoc: { line: 21, col: 88 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "RdpPermissionRequestCoordinator.register", count: 0, regions: { 0: { startLoc: { line: 26, col: 3 }, endLoc: { line: 37, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 33, col: 21 }, endLoc: { line: 35, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 36, col: 5 }, endLoc: { line: 37, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 33, col: 9 }, endLoc: { line: 33, col: 19 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 6 }, 7: { name: "anonymous_5", count: 0, regions: { 0: { startLoc: { line: 28, col: 57 }, endLoc: { line: 32, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 29, col: 7 }, endLoc: { line: 32, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 7 }, 8: { name: "anonymous_6", count: 0, regions: { 0: { startLoc: { line: 29, col: 26 }, endLoc: { line: 31, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 30, col: 9 }, endLoc: { line: 31, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 8 }, 9: { name: "RdpPermissionRequestCoordinator.routes", count: 0, regions: { 0: { startLoc: { line: 39, col: 3 }, endLoc: { line: 70, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 40, col: 5 }, endLoc: { line: 70, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 9 }, 10: { name: "anonymous_7", count: 0, regions: { 0: { startLoc: { line: 45, col: 27 }, endLoc: { line: 46, col: 69 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 10 }, 11: { name: "anonymous_8", count: 0, regions: { 0: { startLoc: { line: 52, col: 27 }, endLoc: { line: 53, col: 65 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 11 }, 12: { name: "anonymous_9", count: 0, regions: { 0: { startLoc: { line: 59, col: 27 }, endLoc: { line: 60, col: 73 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 12 }, 13: { name: "anonymous_10", count: 0, regions: { 0: { startLoc: { line: 66, col: 27 }, endLoc: { line: 67, col: 67 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 13 }, 14: { name: "RdpPermissionRequestCoordinator.findRoute", count: 0, regions: { 0: { startLoc: { line: 72, col: 3 }, endLoc: { line: 79, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 73, col: 5 }, endLoc: { line: 77, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 74, col: 32 }, endLoc: { line: 76, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 78, col: 5 }, endLoc: { line: 79, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 74, col: 11 }, endLoc: { line: 74, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 14 }, 15: { name: "RdpPermissionRequestCoordinator.handleRequest", count: 0, regions: { 0: { startLoc: { line: 81, col: 3 }, endLoc: { line: 98, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 83, col: 30 }, endLoc: { line: 86, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 87, col: 5 }, endLoc: { line: 98, col: 4 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 96, col: 9 }, endLoc: { line: 97, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 83, col: 9 }, endLoc: { line: 83, col: 28 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 88, col: 23 }, endLoc: { line: 88, col: 67 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 15 }, 16: { name: "anonymous_11", count: 0, regions: { 0: { startLoc: { line: 91, col: 13 }, endLoc: { line: 93, col: 8 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 92, col: 9 }, endLoc: { line: 93, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 16 }, 17: { name: "anonymous_12", count: 0, regions: { 0: { startLoc: { line: 94, col: 14 }, endLoc: { line: 97, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 17 }, 18: { name: "RdpPermissionRequestCoordinator.complete", count: 0, regions: { 0: { startLoc: { line: 100, col: 3 }, endLoc: { line: 109, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 106, col: 33 }, endLoc: { line: 108, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 106, col: 9 }, endLoc: { line: 106, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 18 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 10, 9: 11, 10: 12, 11: 13, 12: 14, 13: 17, 14: 18, 15: 19, 16: 21, 17: 22, 18: 23, 19: 26, 20: 27, 21: 28, 22: 29, 23: 30, 24: 33, 25: 34, 26: 36, 27: 39, 28: 40, 29: 41, 30: 42, 31: 43, 32: 44, 33: 45, 34: 46, 35: 48, 36: 49, 37: 50, 38: 51, 39: 52, 40: 53, 41: 55, 42: 56, 43: 57, 44: 58, 45: 59, 46: 60, 47: 62, 48: 63, 49: 64, 50: 65, 51: 66, 52: 67, 53: 72, 54: 73, 55: 74, 56: 75, 57: 78, 58: 81, 59: 82, 60: 83, 61: 84, 62: 85, 63: 87, 64: 88, 65: 89, 66: 90, 67: 91, 68: 92, 69: 94, 70: 95, 71: 96, 72: 100, 73: 101, 74: 102, 75: 103, 76: 104, 77: 106, 78: 107 } });
interface PermissionRoute {
    type: NativePermissionType;
    label: string;
    trigger: string;
    ensurePermission: (trigger: string) => Promise<boolean>;
}
export class RdpPermissionRequestCoordinator {
    private readonly permissionManager: RdpPermissionManager;
    private readonly deferUiUpdate: (task: () => void) => void;
    constructor(permissionManager: RdpPermissionManager, deferUiUpdate: (task: () => void) => void) {
        bjccovmshb1i16.instrumentFunction(3);
        bjccovmshb1i16.instrumentRegion(3, 1);
        this.permissionManager = permissionManager;
        this.deferUiUpdate = deferUiUpdate;
    }
    register(): NativeCommandResult {
        bjccovmshb1i16.instrumentFunction(6);
        const routes = this.routes();
        const result = NativeRdpGateway.onPermissionRequest((request: NativePermissionRequest): void => {
            bjccovmshb1i16.instrumentFunction(7);
            bjccovmshb1i16.instrumentRegion(7, 1);
            this.deferUiUpdate((): void => {
                bjccovmshb1i16.instrumentFunction(8);
                bjccovmshb1i16.instrumentRegion(8, 1);
                this.handleRequest(request, routes);
            });
        });
        if (!result.ok) {
            bjccovmshb1i16.instrumentBranch(6, 0, true);
            bjccovmshb1i16.instrumentRegion(6, 1);
            RdpLogger.error(`Native permission callback registration failed: ${result.message}`);
        }
        else {
            bjccovmshb1i16.instrumentBranch(6, 0, false);
        }
        bjccovmshb1i16.instrumentRegion(6, 2);
        return result;
    }
    private routes(): PermissionRoute[] {
        bjccovmshb1i16.instrumentFunction(9);
        bjccovmshb1i16.instrumentRegion(9, 1);
        return [
            {
                type: 'microphone',
                label: 'Remote audio capture requested microphone permission',
                trigger: 'remote audio capture',
                ensurePermission: (trigger: string): Promise<boolean> => { bjccovmshb1i16.instrumentFunction(10); return this.permissionManager.ensureMicrophonePermission(trigger); }
            },
            {
                type: 'camera',
                label: 'Remote camera redirection requested camera permission',
                trigger: 'remote camera redirection',
                ensurePermission: (trigger: string): Promise<boolean> => { bjccovmshb1i16.instrumentFunction(11); return this.permissionManager.ensureCameraPermission(trigger); }
            },
            {
                type: 'clipboard',
                label: 'Clipboard sync requested pasteboard permission',
                trigger: 'clipboard read',
                ensurePermission: (trigger: string): Promise<boolean> => { bjccovmshb1i16.instrumentFunction(12); return this.permissionManager.ensurePasteboardReadPermission(trigger); }
            },
            {
                type: 'location',
                label: 'RDP location channel requested location permission',
                trigger: 'remote location redirection',
                ensurePermission: (trigger: string): Promise<boolean> => { bjccovmshb1i16.instrumentFunction(13); return this.permissionManager.ensureLocationPermission(trigger); }
            }
        ];
    }
    private findRoute(type: NativePermissionType, routes: PermissionRoute[]): PermissionRoute | undefined {
        bjccovmshb1i16.instrumentFunction(14);
        for (const route of routes) {
            bjccovmshb1i16.instrumentRegion(14, 1);
            if (route.type === type) {
                bjccovmshb1i16.instrumentBranch(14, 0, true);
                bjccovmshb1i16.instrumentRegion(14, 2);
                return route;
            }
            else {
                bjccovmshb1i16.instrumentBranch(14, 0, false);
            }
        }
        bjccovmshb1i16.instrumentRegion(14, 3);
        return undefined;
    }
    private handleRequest(request: NativePermissionRequest, routes: PermissionRoute[]): void {
        bjccovmshb1i16.instrumentFunction(15);
        const route = this.findRoute(request.type, routes);
        if (route === undefined) {
            bjccovmshb1i16.instrumentBranch(15, 0, true);
            bjccovmshb1i16.instrumentRegion(15, 1);
            RdpLogger.error(`Unknown native permission request type: ${request.type}`);
            return;
        }
        else {
            bjccovmshb1i16.instrumentBranch(15, 0, false);
        }
        bjccovmshb1i16.instrumentRegion(15, 2);
        const parsedRequestId = Number(request.requestId);
        const requestId = isNaN(parsedRequestId) ? (bjccovmshb1i16.instrumentBranch(15, 1, true), 0) : (bjccovmshb1i16.instrumentBranch(15, 1, false), parsedRequestId);
        RdpLogger.info(`${route.label}: requestId=${request.requestId}`);
        route.ensurePermission(route.trigger)
            .then((granted: boolean): void => {
            bjccovmshb1i16.instrumentFunction(16);
            bjccovmshb1i16.instrumentRegion(16, 1);
            this.complete(request.type, requestId, granted);
        })
            .catch((error: Error): void => {
            bjccovmshb1i16.instrumentFunction(17);
            RdpLogger.error(`${route.label} failed: ${JSON.stringify(error)}`);
            bjccovmshb1i16.instrumentRegion(15, 3);
            this.complete(request.type, requestId, false);
        });
    }
    private complete(type: NativePermissionType, requestId: number, granted: boolean): void {
        bjccovmshb1i16.instrumentFunction(18);
        const result = NativeRdpGateway.completePermissionRequest({
            type: type,
            requestId: requestId,
            granted: granted
        });
        if (!granted || !result.ok) {
            bjccovmshb1i16.instrumentBranch(18, 0, true);
            bjccovmshb1i16.instrumentRegion(18, 1);
            RdpLogger.warn(`${result.message}: granted=${granted}`);
        }
        else {
            bjccovmshb1i16.instrumentBranch(18, 0, false);
        }
    }
}
