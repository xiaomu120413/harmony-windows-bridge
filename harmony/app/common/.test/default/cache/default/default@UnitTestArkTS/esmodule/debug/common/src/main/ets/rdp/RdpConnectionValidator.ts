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
let bjccovmshb1i0n = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/rdp/RdpConnectionValidator.ets", hash: "ea43c9e3e8e6fe3cf005e9c58443f6df6ed459d6fc924c222ba0521185e88fc2", lineCnt: 114, count: 0, projectPath: "", functions: { 0: { name: "RdpConnectionValidator.validate", count: 0, regions: { 0: { startLoc: { line: 15, col: 3 }, endLoc: { line: 43, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 22, col: 29 }, endLoc: { line: 24, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 26, col: 29 }, endLoc: { line: 28, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 29, col: 32 }, endLoc: { line: 31, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 32, col: 39 }, endLoc: { line: 34, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 35, col: 5 }, endLoc: { line: 43, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 22, col: 9 }, endLoc: { line: 22, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 26, col: 9 }, endLoc: { line: 26, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 29, col: 9 }, endLoc: { line: 29, col: 30 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 32, col: 9 }, endLoc: { line: 32, col: 37 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 }, 1: { name: "RdpConnectionValidator.validateHost", count: 0, regions: { 0: { startLoc: { line: 45, col: 3 }, endLoc: { line: 65, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 46, col: 28 }, endLoc: { line: 48, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 49, col: 28 }, endLoc: { line: 51, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 52, col: 26 }, endLoc: { line: 54, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 55, col: 88 }, endLoc: { line: 57, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 58, col: 33 }, endLoc: { line: 60, col: 6 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 61, col: 55 }, endLoc: { line: 63, col: 6 }, count: 0, ignored: 0 }, 7: { startLoc: { line: 64, col: 5 }, endLoc: { line: 65, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 46, col: 9 }, endLoc: { line: 46, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 49, col: 9 }, endLoc: { line: 49, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 52, col: 9 }, endLoc: { line: 52, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 55, col: 9 }, endLoc: { line: 55, col: 86 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 58, col: 9 }, endLoc: { line: 58, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 59, col: 14 }, endLoc: { line: 59, col: 80 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 6: { startLoc: { line: 61, col: 9 }, endLoc: { line: 61, col: 53 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 7: { startLoc: { line: 62, col: 14 }, endLoc: { line: 62, col: 82 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 8: { startLoc: { line: 64, col: 12 }, endLoc: { line: 64, col: 77 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 1 }, 2: { name: "RdpConnectionValidator.validatePort", count: 0, regions: { 0: { startLoc: { line: 67, col: 3 }, endLoc: { line: 76, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 68, col: 28 }, endLoc: { line: 70, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 71, col: 33 }, endLoc: { line: 73, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 74, col: 5 }, endLoc: { line: 76, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 68, col: 9 }, endLoc: { line: 68, col: 26 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 71, col: 9 }, endLoc: { line: 71, col: 31 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 75, col: 12 }, endLoc: { line: 75, col: 69 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 2 }, 3: { name: "RdpConnectionValidator.isIpv4Candidate", count: 0, regions: { 0: { startLoc: { line: 78, col: 3 }, endLoc: { line: 80, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 79, col: 5 }, endLoc: { line: 80, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 3 }, 4: { name: "RdpConnectionValidator.isValidIpv4Host", count: 0, regions: { 0: { startLoc: { line: 82, col: 3 }, endLoc: { line: 97, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 84, col: 29 }, endLoc: { line: 86, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 87, col: 5 }, endLoc: { line: 95, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 88, col: 56 }, endLoc: { line: 90, col: 8 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 92, col: 68 }, endLoc: { line: 94, col: 8 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 96, col: 5 }, endLoc: { line: 97, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 84, col: 9 }, endLoc: { line: 84, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 88, col: 11 }, endLoc: { line: 88, col: 54 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 92, col: 11 }, endLoc: { line: 92, col: 66 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 4 }, 5: { name: "RdpConnectionValidator.isValidIpv6Host", count: 0, regions: { 0: { startLoc: { line: 99, col: 3 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 100, col: 5 }, endLoc: { line: 101, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "RdpConnectionValidator.isValidDnsHost", count: 0, regions: { 0: { startLoc: { line: 103, col: 3 }, endLoc: { line: 112, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 106, col: 5 }, endLoc: { line: 110, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 107, col: 81 }, endLoc: { line: 109, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 111, col: 5 }, endLoc: { line: 112, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 107, col: 11 }, endLoc: { line: 107, col: 79 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 6 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 5, 4: 6, 5: 7, 6: 8, 7: 9, 8: 10, 9: 11, 10: 14, 11: 15, 12: 16, 13: 17, 14: 18, 15: 19, 16: 20, 17: 21, 18: 22, 19: 23, 20: 25, 21: 26, 22: 27, 23: 29, 24: 30, 25: 32, 26: 33, 27: 35, 28: 36, 29: 37, 30: 38, 31: 39, 32: 40, 33: 41, 34: 45, 35: 46, 36: 47, 37: 49, 38: 50, 39: 52, 40: 53, 41: 55, 42: 56, 43: 58, 44: 59, 45: 61, 46: 62, 47: 64, 48: 67, 49: 68, 50: 69, 51: 71, 52: 72, 53: 74, 54: 75, 55: 78, 56: 79, 57: 82, 58: 83, 59: 84, 60: 85, 61: 87, 62: 88, 63: 89, 64: 91, 65: 92, 66: 93, 67: 96, 68: 99, 69: 100, 70: 103, 71: 104, 72: 105, 73: 106, 74: 107, 75: 108, 76: 111 } });
export type ConnectionValidationIssue = 'hostRequired' | 'hostTooLong' | 'hostSpaces' | 'hostProtocolOrPath' | 'hostFormat' | 'hostIpFormat' | 'portRequired' | 'portDigits' | 'portRange' | 'usernameRequired' | 'passwordRequired';
export interface ConnectionValidationResult {
    ok: boolean;
    issues: ConnectionValidationIssue[];
    host: string;
    port: string;
    username: string;
    password: string;
}
export class RdpConnectionValidator {
    static validate(hostInput: string, portInput: string, usernameInput: string, password: string): ConnectionValidationResult {
        bjccovmshb1i0n.instrumentFunction(0);
        const host = hostInput.trim();
        const port = portInput.trim();
        const username = usernameInput.trim();
        const issues: ConnectionValidationIssue[] = [];
        const hostIssue = RdpConnectionValidator.validateHost(host);
        if (hostIssue !== null) {
            bjccovmshb1i0n.instrumentBranch(0, 0, true);
            bjccovmshb1i0n.instrumentRegion(0, 1);
            issues.push(hostIssue);
        }
        else {
            bjccovmshb1i0n.instrumentBranch(0, 0, false);
        }
        const portIssue = RdpConnectionValidator.validatePort(port);
        if (portIssue !== null) {
            bjccovmshb1i0n.instrumentBranch(0, 1, true);
            bjccovmshb1i0n.instrumentRegion(0, 2);
            issues.push(portIssue);
        }
        else {
            bjccovmshb1i0n.instrumentBranch(0, 1, false);
        }
        if (username.length === 0) {
            bjccovmshb1i0n.instrumentBranch(0, 2, true);
            bjccovmshb1i0n.instrumentRegion(0, 3);
            issues.push('usernameRequired');
        }
        else {
            bjccovmshb1i0n.instrumentBranch(0, 2, false);
        }
        if (password.trim().length === 0) {
            bjccovmshb1i0n.instrumentBranch(0, 3, true);
            bjccovmshb1i0n.instrumentRegion(0, 4);
            issues.push('passwordRequired');
        }
        else {
            bjccovmshb1i0n.instrumentBranch(0, 3, false);
        }
        bjccovmshb1i0n.instrumentRegion(0, 5);
        return {
            ok: issues.length === 0,
            issues: issues,
            host: host,
            port: port,
            username: username,
            password: password
        };
    }
    private static validateHost(host: string): ConnectionValidationIssue | null {
        bjccovmshb1i0n.instrumentFunction(1);
        if (host.length === 0) {
            bjccovmshb1i0n.instrumentBranch(1, 0, true);
            bjccovmshb1i0n.instrumentRegion(1, 1);
            return 'hostRequired';
        }
        else {
            bjccovmshb1i0n.instrumentBranch(1, 0, false);
        }
        if (host.length > 253) {
            bjccovmshb1i0n.instrumentBranch(1, 1, true);
            bjccovmshb1i0n.instrumentRegion(1, 2);
            return 'hostTooLong';
        }
        else {
            bjccovmshb1i0n.instrumentBranch(1, 1, false);
        }
        if (/\s/.test(host)) {
            bjccovmshb1i0n.instrumentBranch(1, 2, true);
            bjccovmshb1i0n.instrumentRegion(1, 3);
            return 'hostSpaces';
        }
        else {
            bjccovmshb1i0n.instrumentBranch(1, 2, false);
        }
        if (host.indexOf('/') >= 0 || host.indexOf('\\') >= 0 || host.indexOf('://') >= 0) {
            bjccovmshb1i0n.instrumentBranch(1, 3, true);
            bjccovmshb1i0n.instrumentRegion(1, 4);
            return 'hostProtocolOrPath';
        }
        else {
            bjccovmshb1i0n.instrumentBranch(1, 3, false);
        }
        if (host.indexOf(':') >= 0) {
            bjccovmshb1i0n.instrumentBranch(1, 4, true);
            bjccovmshb1i0n.instrumentRegion(1, 5);
            return RdpConnectionValidator.isValidIpv6Host(host) ? (bjccovmshb1i0n.instrumentBranch(1, 5, true), null) : (bjccovmshb1i0n.instrumentBranch(1, 5, false), 'hostFormat');
        }
        else {
            bjccovmshb1i0n.instrumentBranch(1, 4, false);
        }
        if (RdpConnectionValidator.isIpv4Candidate(host)) {
            bjccovmshb1i0n.instrumentBranch(1, 6, true);
            bjccovmshb1i0n.instrumentRegion(1, 6);
            return RdpConnectionValidator.isValidIpv4Host(host) ? (bjccovmshb1i0n.instrumentBranch(1, 7, true), null) : (bjccovmshb1i0n.instrumentBranch(1, 7, false), 'hostIpFormat');
        }
        else {
            bjccovmshb1i0n.instrumentBranch(1, 6, false);
        }
        bjccovmshb1i0n.instrumentRegion(1, 7);
        return RdpConnectionValidator.isValidDnsHost(host) ? (bjccovmshb1i0n.instrumentBranch(1, 8, true), null) : (bjccovmshb1i0n.instrumentBranch(1, 8, false), 'hostFormat');
    }
    private static validatePort(port: string): ConnectionValidationIssue | null {
        bjccovmshb1i0n.instrumentFunction(2);
        if (port.length === 0) {
            bjccovmshb1i0n.instrumentBranch(2, 0, true);
            bjccovmshb1i0n.instrumentRegion(2, 1);
            return 'portRequired';
        }
        else {
            bjccovmshb1i0n.instrumentBranch(2, 0, false);
        }
        if (!/^[0-9]+$/.test(port)) {
            bjccovmshb1i0n.instrumentBranch(2, 1, true);
            bjccovmshb1i0n.instrumentRegion(2, 2);
            return 'portDigits';
        }
        else {
            bjccovmshb1i0n.instrumentBranch(2, 1, false);
        }
        bjccovmshb1i0n.instrumentRegion(2, 3);
        const portNumber = Number(port);
        return portNumber < 1 || portNumber > 65535 ? (bjccovmshb1i0n.instrumentBranch(2, 2, true), 'portRange') : (bjccovmshb1i0n.instrumentBranch(2, 2, false), null);
    }
    private static isIpv4Candidate(host: string): boolean {
        bjccovmshb1i0n.instrumentFunction(3);
        bjccovmshb1i0n.instrumentRegion(3, 1);
        return /^[0-9.]+$/.test(host) && host.indexOf('.') >= 0;
    }
    private static isValidIpv4Host(host: string): boolean {
        bjccovmshb1i0n.instrumentFunction(4);
        const parts = host.split('.');
        if (parts.length !== 4) {
            bjccovmshb1i0n.instrumentBranch(4, 0, true);
            bjccovmshb1i0n.instrumentRegion(4, 1);
            return false;
        }
        else {
            bjccovmshb1i0n.instrumentBranch(4, 0, false);
        }
        for (const part of parts) {
            bjccovmshb1i0n.instrumentRegion(4, 2);
            if (part.length === 0 || !/^[0-9]+$/.test(part)) {
                bjccovmshb1i0n.instrumentBranch(4, 1, true);
                bjccovmshb1i0n.instrumentRegion(4, 3);
                return false;
            }
            else {
                bjccovmshb1i0n.instrumentBranch(4, 1, false);
            }
            const value = Number(part);
            if (value < 0 || value > 255 || Math.floor(value) !== value) {
                bjccovmshb1i0n.instrumentBranch(4, 2, true);
                bjccovmshb1i0n.instrumentRegion(4, 4);
                return false;
            }
            else {
                bjccovmshb1i0n.instrumentBranch(4, 2, false);
            }
        }
        bjccovmshb1i0n.instrumentRegion(4, 5);
        return true;
    }
    private static isValidIpv6Host(host: string): boolean {
        bjccovmshb1i0n.instrumentFunction(5);
        bjccovmshb1i0n.instrumentRegion(5, 1);
        return /^[0-9A-Fa-f:]+$/.test(host) && host.indexOf(':') >= 0 && host.split(':').length >= 3;
    }
    private static isValidDnsHost(host: string): boolean {
        bjccovmshb1i0n.instrumentFunction(6);
        const labels = host.split('.');
        const labelPattern = /^[A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?$/;
        for (const label of labels) {
            bjccovmshb1i0n.instrumentRegion(6, 1);
            if (label.length === 0 || label.length > 63 || !labelPattern.test(label)) {
                bjccovmshb1i0n.instrumentBranch(6, 0, true);
                bjccovmshb1i0n.instrumentRegion(6, 2);
                return false;
            }
            else {
                bjccovmshb1i0n.instrumentBranch(6, 0, false);
            }
        }
        bjccovmshb1i0n.instrumentRegion(6, 3);
        return true;
    }
}
