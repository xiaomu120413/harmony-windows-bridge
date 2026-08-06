import type { ConnectionValidationIssue } from '../../rdp/RdpConnectionValidator';
import { HomeText } from "@normalized:N&&&common/src/main/ets/components/home/HomeText&";
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
let bjccovmshb1i7n = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/home/HomeConnectionValidation.ets", hash: "c5d80037839016cf9f0fc71fa513d895f81667574c0f5f8d8a68b6de277c9cba", lineCnt: 28, count: 0, projectPath: "", functions: { 0: { name: "HomeConnectionValidation.message", count: 0, regions: { 0: { startLoc: { line: 5, col: 3 }, endLoc: { line: 11, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 7, col: 5 }, endLoc: { line: 9, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 10, col: 5 }, endLoc: { line: 11, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "HomeConnectionValidation.issueMessage", count: 0, regions: { 0: { startLoc: { line: 13, col: 3 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 14, col: 35 }, endLoc: { line: 14, col: 64 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 15, col: 34 }, endLoc: { line: 15, col: 63 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 16, col: 33 }, endLoc: { line: 16, col: 63 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 17, col: 41 }, endLoc: { line: 17, col: 81 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 18, col: 33 }, endLoc: { line: 18, col: 68 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 19, col: 35 }, endLoc: { line: 19, col: 73 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 20, col: 35 }, endLoc: { line: 20, col: 64 }, count: 0, ignored: 0 }, 8: { startLoc: { line: 21, col: 33 }, endLoc: { line: 21, col: 65 }, count: 0, ignored: 0 }, 9: { startLoc: { line: 22, col: 32 }, endLoc: { line: 22, col: 66 }, count: 0, ignored: 0 }, 10: { startLoc: { line: 23, col: 39 }, endLoc: { line: 23, col: 72 }, count: 0, ignored: 0 }, 11: { startLoc: { line: 24, col: 39 }, endLoc: { line: 24, col: 72 }, count: 0, ignored: 0 }, 12: { startLoc: { line: 25, col: 5 }, endLoc: { line: 26, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 14, col: 9 }, endLoc: { line: 14, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 15, col: 9 }, endLoc: { line: 15, col: 32 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 16, col: 9 }, endLoc: { line: 16, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 17, col: 9 }, endLoc: { line: 17, col: 39 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 18, col: 9 }, endLoc: { line: 18, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 19, col: 9 }, endLoc: { line: 19, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 20, col: 9 }, endLoc: { line: 20, col: 33 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 21, col: 9 }, endLoc: { line: 21, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 8: { startLoc: { line: 22, col: 9 }, endLoc: { line: 22, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 9: { startLoc: { line: 23, col: 9 }, endLoc: { line: 23, col: 37 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 10: { startLoc: { line: 24, col: 9 }, endLoc: { line: 24, col: 37 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 1 } }, exeLine: { 0: 1, 1: 2, 2: 4, 3: 5, 4: 6, 5: 7, 6: 8, 7: 10, 8: 13, 9: 14, 10: 15, 11: 16, 12: 17, 13: 18, 14: 19, 15: 20, 16: 21, 17: 22, 18: 23, 19: 24, 20: 25 } });
export class HomeConnectionValidation {
    static message(issues: ConnectionValidationIssue[]): string {
        bjccovmshb1i7n.instrumentFunction(0);
        const messages: string[] = [];
        for (const issue of issues) {
            bjccovmshb1i7n.instrumentRegion(0, 1);
            messages.push(HomeConnectionValidation.issueMessage(issue));
        }
        bjccovmshb1i7n.instrumentRegion(0, 2);
        return messages.join(HomeText.VALIDATION_SEPARATOR);
    }
    private static issueMessage(issue: ConnectionValidationIssue): string {
        bjccovmshb1i7n.instrumentFunction(1);
        if (issue === 'hostRequired') {
            bjccovmshb1i7n.instrumentBranch(1, 0, true);
            bjccovmshb1i7n.instrumentRegion(1, 1);
            return HomeText.HOST_REQUIRED;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 0, false);
        }
        if (issue === 'hostTooLong') {
            bjccovmshb1i7n.instrumentBranch(1, 1, true);
            bjccovmshb1i7n.instrumentRegion(1, 2);
            return HomeText.HOST_TOO_LONG;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 1, false);
        }
        if (issue === 'hostSpaces') {
            bjccovmshb1i7n.instrumentBranch(1, 2, true);
            bjccovmshb1i7n.instrumentRegion(1, 3);
            return HomeText.HOST_NO_SPACES;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 2, false);
        }
        if (issue === 'hostProtocolOrPath') {
            bjccovmshb1i7n.instrumentBranch(1, 3, true);
            bjccovmshb1i7n.instrumentRegion(1, 4);
            return HomeText.HOST_NO_PROTOCOL_OR_PATH;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 3, false);
        }
        if (issue === 'hostFormat') {
            bjccovmshb1i7n.instrumentBranch(1, 4, true);
            bjccovmshb1i7n.instrumentRegion(1, 5);
            return HomeText.HOST_FORMAT_INVALID;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 4, false);
        }
        if (issue === 'hostIpFormat') {
            bjccovmshb1i7n.instrumentBranch(1, 5, true);
            bjccovmshb1i7n.instrumentRegion(1, 6);
            return HomeText.HOST_IP_FORMAT_INVALID;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 5, false);
        }
        if (issue === 'portRequired') {
            bjccovmshb1i7n.instrumentBranch(1, 6, true);
            bjccovmshb1i7n.instrumentRegion(1, 7);
            return HomeText.PORT_REQUIRED;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 6, false);
        }
        if (issue === 'portDigits') {
            bjccovmshb1i7n.instrumentBranch(1, 7, true);
            bjccovmshb1i7n.instrumentRegion(1, 8);
            return HomeText.PORT_DIGITS_ONLY;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 7, false);
        }
        if (issue === 'portRange') {
            bjccovmshb1i7n.instrumentBranch(1, 8, true);
            bjccovmshb1i7n.instrumentRegion(1, 9);
            return HomeText.PORT_RANGE_INVALID;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 8, false);
        }
        if (issue === 'usernameRequired') {
            bjccovmshb1i7n.instrumentBranch(1, 9, true);
            bjccovmshb1i7n.instrumentRegion(1, 10);
            return HomeText.USERNAME_REQUIRED;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 9, false);
        }
        if (issue === 'passwordRequired') {
            bjccovmshb1i7n.instrumentBranch(1, 10, true);
            bjccovmshb1i7n.instrumentRegion(1, 11);
            return HomeText.PASSWORD_REQUIRED;
        }
        else {
            bjccovmshb1i7n.instrumentBranch(1, 10, false);
        }
        bjccovmshb1i7n.instrumentRegion(1, 12);
        return HomeText.CONNECTION_FAILURE;
    }
}
