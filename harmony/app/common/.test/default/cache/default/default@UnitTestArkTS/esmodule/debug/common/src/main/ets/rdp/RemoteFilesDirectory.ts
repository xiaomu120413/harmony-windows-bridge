import type common from "@ohos:app.ability.common";
import type Want from "@ohos:app.ability.Want";
import wantConstant from "@ohos:app.ability.wantConstant";
import Environment from "@ohos:file.environment";
import fileIo from "@ohos:file.fs";
import fileUri from "@ohos:file.fileuri";
import { REMOTE_FILES_SHARED_DIR_NAME, REMOTE_FILES_SHARED_DIR_STORAGE_KEY } from "@normalized:N&&&common/src/main/ets/components/settings/SettingsConstants&";
import { RdpLogger } from "@normalized:N&&&common/src/main/ets/rdp/RdpLogger&";
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
let bjccovmshb1i29 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/RemoteFilesDirectory.ets", hash: "c3bc960b24776eb2560bad89f7af7804b31b8aa9f1a4f71c471fe50496ef0e81", lineCnt: 94, count: 0, projectPath: "", functions: { 0: { name: "RemoteFilesDirectory.prepareSharedDirectory", count: 0, regions: { 0: { startLoc: { line: 15, col: 3 }, endLoc: { line: 34, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 16, col: 9 }, endLoc: { line: 31, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 31, col: 7 }, endLoc: { line: 33, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 18, col: 37 }, endLoc: { line: 21, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 24, col: 56 }, endLoc: { line: 27, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 29, col: 7 }, endLoc: { line: 31, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 18, col: 11 }, endLoc: { line: 18, col: 35 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 24, col: 11 }, endLoc: { line: 24, col: 54 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 }, 1: { name: "RemoteFilesDirectory.openSharedDirectory", count: 0, regions: { 0: { startLoc: { line: 36, col: 3 }, endLoc: { line: 53, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 37, col: 27 }, endLoc: { line: 40, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 43, col: 55 }, endLoc: { line: 46, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 48, col: 9 }, endLoc: { line: 50, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 50, col: 7 }, endLoc: { line: 52, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 37, col: 9 }, endLoc: { line: 37, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 43, col: 9 }, endLoc: { line: 43, col: 53 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 1 }, 2: { name: "RemoteFilesDirectory.openDirectoryWant", count: 0, regions: { 0: { startLoc: { line: 55, col: 3 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 56, col: 5 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "RemoteFilesDirectory.sharedDirectoryPath", count: 0, regions: { 0: { startLoc: { line: 71, col: 3 }, endLoc: { line: 76, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 72, col: 96 }, endLoc: { line: 74, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 75, col: 5 }, endLoc: { line: 76, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 72, col: 9 }, endLoc: { line: 72, col: 94 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 3 }, 4: { name: "RemoteFilesDirectory.isDirectory", count: 0, regions: { 0: { startLoc: { line: 78, col: 3 }, endLoc: { line: 84, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 79, col: 9 }, endLoc: { line: 81, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 81, col: 7 }, endLoc: { line: 83, col: 6 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "RemoteFilesDirectory.joinPath", count: 0, regions: { 0: { startLoc: { line: 86, col: 3 }, endLoc: { line: 88, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 87, col: 5 }, endLoc: { line: 88, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 87, col: 12 }, endLoc: { line: 87, col: 77 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 5 }, 6: { name: "RemoteFilesDirectory.endsWithPathComponent", count: 0, regions: { 0: { startLoc: { line: 90, col: 3 }, endLoc: { line: 92, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 91, col: 5 }, endLoc: { line: 92, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 9, 8: 10, 9: 11, 10: 12, 11: 14, 12: 15, 13: 16, 14: 17, 15: 18, 16: 19, 17: 20, 18: 23, 19: 24, 20: 25, 21: 26, 22: 29, 23: 30, 24: 31, 25: 32, 26: 36, 27: 37, 28: 38, 29: 39, 30: 42, 31: 43, 32: 44, 33: 45, 34: 48, 35: 49, 36: 50, 37: 51, 38: 55, 39: 56, 40: 57, 41: 58, 42: 59, 43: 61, 44: 62, 45: 63, 46: 64, 47: 65, 48: 66, 49: 67, 50: 71, 51: 72, 52: 73, 53: 75, 54: 78, 55: 79, 56: 80, 57: 81, 58: 82, 59: 86, 60: 87, 61: 90, 62: 91 } });
const FILE_MANAGER_BUNDLE_NAME = 'com.huawei.hmos.filemanager';
const FILE_MANAGER_MAIN_ABILITY = 'MainAbility';
const FILE_MANAGER_OPEN_DIRECTORY_MODE = 'openDirectory';
const FILE_MANAGER_OPEN_DIRECTORY_URI = 'filemanager://openDirectory';
export class RemoteFilesDirectory {
    static prepareSharedDirectory(): void {
        bjccovmshb1i29.instrumentFunction(0);
        try {
            bjccovmshb1i29.instrumentRegion(0, 1);
            const downloadDir = Environment.getUserDownloadDir();
            if (downloadDir.length === 0) {
                bjccovmshb1i29.instrumentBranch(0, 0, true);
                bjccovmshb1i29.instrumentRegion(0, 3);
                RdpLogger.warn('prepare download drive directory skipped: Download directory is empty');
                return;
            }
            else {
                bjccovmshb1i29.instrumentBranch(0, 0, false);
            }
            const targetDir = RemoteFilesDirectory.sharedDirectoryPath(downloadDir);
            if (RemoteFilesDirectory.isDirectory(targetDir)) {
                bjccovmshb1i29.instrumentBranch(0, 1, true);
                bjccovmshb1i29.instrumentRegion(0, 4);
                AppStorage.setOrCreate<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY, targetDir);
                return;
            }
            else {
                bjccovmshb1i29.instrumentBranch(0, 1, false);
            }
            bjccovmshb1i29.instrumentRegion(0, 5);
            fileIo.mkdirSync(targetDir);
            AppStorage.setOrCreate<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY, targetDir);
        }
        catch (exception) {
            bjccovmshb1i29.instrumentRegion(0, 2);
            RdpLogger.error(`prepare download drive directory failed: ${JSON.stringify(exception)}`);
        }
    }
    static async openSharedDirectory(context: common.UIAbilityContext | null): Promise<void> {
        bjccovmshb1i29.instrumentFunction(1);
        if (context === null) {
            bjccovmshb1i29.instrumentBranch(1, 0, true);
            bjccovmshb1i29.instrumentRegion(1, 1);
            RdpLogger.warn('remote files directory open skipped: no UIAbilityContext');
            return;
        }
        else {
            bjccovmshb1i29.instrumentBranch(1, 0, false);
        }
        const storedDir = AppStorage.get<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY) || '';
        if (!RemoteFilesDirectory.isDirectory(storedDir)) {
            bjccovmshb1i29.instrumentBranch(1, 1, true);
            bjccovmshb1i29.instrumentRegion(1, 2);
            RdpLogger.warn('remote files directory open skipped: shared directory is not ready');
            return;
        }
        else {
            bjccovmshb1i29.instrumentBranch(1, 1, false);
        }
        try {
            bjccovmshb1i29.instrumentRegion(1, 3);
            await context.startAbility(RemoteFilesDirectory.openDirectoryWant(storedDir));
        }
        catch (exception) {
            bjccovmshb1i29.instrumentRegion(1, 4);
            RdpLogger.error(`remote files directory startAbility failed: ${JSON.stringify(exception)}`);
        }
    }
    private static openDirectoryWant(path: string): Want {
        bjccovmshb1i29.instrumentFunction(2);
        bjccovmshb1i29.instrumentRegion(2, 1);
        const directoryUri = new fileUri.FileUri(path).toString();
        const parametersObject: Object = new Object();
        Reflect.set(parametersObject, 'fileManagerMode', FILE_MANAGER_OPEN_DIRECTORY_MODE);
        Reflect.set(parametersObject, 'fileUri', directoryUri);
        return {
            bundleName: FILE_MANAGER_BUNDLE_NAME,
            abilityName: FILE_MANAGER_MAIN_ABILITY,
            action: 'ohos.want.action.viewData',
            uri: FILE_MANAGER_OPEN_DIRECTORY_URI,
            flags: wantConstant.Flags.FLAG_AUTH_READ_URI_PERMISSION | wantConstant.Flags.FLAG_AUTH_WRITE_URI_PERMISSION,
            parameters: parametersObject as Record<string, Object>
        };
    }
    private static sharedDirectoryPath(downloadDir: string): string {
        bjccovmshb1i29.instrumentFunction(3);
        if (RemoteFilesDirectory.endsWithPathComponent(downloadDir, REMOTE_FILES_SHARED_DIR_NAME)) {
            bjccovmshb1i29.instrumentBranch(3, 0, true);
            bjccovmshb1i29.instrumentRegion(3, 1);
            return downloadDir;
        }
        else {
            bjccovmshb1i29.instrumentBranch(3, 0, false);
        }
        bjccovmshb1i29.instrumentRegion(3, 2);
        return RemoteFilesDirectory.joinPath(downloadDir, REMOTE_FILES_SHARED_DIR_NAME);
    }
    private static isDirectory(path: string): boolean {
        bjccovmshb1i29.instrumentFunction(4);
        try {
            bjccovmshb1i29.instrumentRegion(4, 1);
            return path.length > 0 && fileIo.statSync(path).isDirectory();
        }
        catch (_exception) {
            bjccovmshb1i29.instrumentRegion(4, 2);
            return false;
        }
    }
    private static joinPath(parent: string, child: string): string {
        bjccovmshb1i29.instrumentFunction(5);
        bjccovmshb1i29.instrumentRegion(5, 1);
        return parent.endsWith('/') ? (bjccovmshb1i29.instrumentBranch(5, 0, true), `${parent}${child}`) : (bjccovmshb1i29.instrumentBranch(5, 0, false), `${parent}/${child}`);
    }
    private static endsWithPathComponent(path: string, component: string): boolean {
        bjccovmshb1i29.instrumentFunction(6);
        bjccovmshb1i29.instrumentRegion(6, 1);
        return path === component || path.endsWith(`/${component}`);
    }
}
