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
let bjccovmshb1i7s = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/home/HomeText.ets", hash: "7275b2d367327fca273c80ec9fc92d7631c94b9f1ba14bdaf95823b2d60b8121", lineCnt: 71, count: 0, projectPath: "", functions: { 0: { name: "HomeText.connectionFailureWithDetail", count: 0, regions: { 0: { startLoc: { line: 67, col: 3 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 68, col: 5 }, endLoc: { line: 69, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 } }, exeLine: { 0: 1, 1: 2, 2: 4, 3: 5, 4: 6, 5: 7, 6: 8, 7: 9, 8: 10, 9: 12, 10: 13, 11: 14, 12: 15, 13: 16, 14: 17, 15: 18, 16: 19, 17: 20, 18: 22, 19: 23, 20: 24, 21: 25, 22: 26, 23: 28, 24: 29, 25: 30, 26: 31, 27: 33, 28: 34, 29: 35, 30: 36, 31: 37, 32: 39, 33: 40, 34: 41, 35: 42, 36: 43, 37: 44, 38: 45, 39: 46, 40: 47, 41: 48, 42: 49, 43: 50, 44: 52, 45: 53, 46: 54, 47: 55, 48: 56, 49: 57, 50: 58, 51: 59, 52: 60, 53: 61, 54: 62, 55: 63, 56: 64, 57: 65, 58: 67, 59: 68 } });
export class HomeText {
    static readonly APP_TAGLINE: string = 'HarmonyOS 与 Windows 互联的局域网远程桌面工具';
    static readonly CONNECTION_DETAILS_TITLE: string = '连接详情';
    static readonly WINDOWS_USERNAME_LABEL: string = 'Windows 用户名';
    static readonly WINDOWS_USERNAME_PLACEHOLDER: string = '远程主机名\\用户名，例如：DESKTOP-ABC\\zhangsan';
    static readonly REMEMBER_PASSWORD: string = '记住密码';
    static readonly DEVICE_ACTIONS: string = '设备操作';
    static readonly DELETE_ACTION: string = '删除';
    static readonly CLEAR_PASSWORD_ACTION: string = '清除密码';
    static readonly DEVICE_LIST_TITLE: string = '设备列表';
    static readonly NEW_DEVICE_ACTION: string = '新建设备';
    static readonly DEVICE_SEARCH_PLACEHOLDER: string = '搜索设备名称或地址';
    static readonly DEVICE_TAG_RECENT: string = '最近连接';
    static readonly DEVICE_TAG_PASSWORD_SAVED: string = '已保存密码';
    static readonly DEVICE_EMPTY_TITLE: string = '还没有保存的设备';
    static readonly DEVICE_EMPTY_BODY: string = '连接成功后会自动出现在这里';
    static readonly DEVICE_SEARCH_EMPTY_TITLE: string = '没有匹配设备';
    static readonly DEVICE_SEARCH_EMPTY_BODY: string = '换个设备名称或地址再试';
    static readonly STATUS_SERVICE_LABEL: string = '被控服务';
    static readonly STATUS_SCREEN_LABEL: string = '录屏权限';
    static readonly STATUS_INPUT_LABEL: string = '注入权限';
    static readonly STATUS_FILES_LABEL: string = '共享目录';
    static readonly STATUS_FILES_CONFIGURED: string = '已配置';
    static readonly PASSWORD_CLEARED_CURRENT: string = '已清空当前密码';
    static readonly PASSWORD_CLEAR_STORAGE_UNAVAILABLE: string = '清除密码失败：存储不可用';
    static readonly PASSWORD_CLEARED_SAVED: string = '已清除保存的密码';
    static readonly PASSWORD_CLEAR_FAILED: string = '清除密码失败';
    static readonly CONNECTION_FAILURE: string = '连接失败';
    static readonly CONNECTION_FAILURE_CREDENTIALS: string = '连接失败，请检查 Host、账号或密码';
    static readonly CONNECTION_FAILURE_NO_SESSION: string = '连接失败，未建立远程桌面会话';
    static readonly CONNECTION_SUCCESS: string = '连接成功';
    static readonly CONNECTION_CONNECTING: string = '正在连接...';
    static readonly USERNAME_REQUIRED: string = 'Username 必填';
    static readonly PASSWORD_REQUIRED: string = 'Password 必填';
    static readonly HOST_REQUIRED: string = 'Host 必填';
    static readonly HOST_TOO_LONG: string = 'Host 过长';
    static readonly HOST_NO_SPACES: string = 'Host 不能含空格';
    static readonly HOST_NO_PROTOCOL_OR_PATH: string = 'Host 不要包含协议或路径';
    static readonly HOST_FORMAT_INVALID: string = 'Host 格式错误';
    static readonly HOST_IP_FORMAT_INVALID: string = 'Host IP 格式错误';
    static readonly PORT_REQUIRED: string = 'Port 必填';
    static readonly PORT_DIGITS_ONLY: string = 'Port 只能是数字';
    static readonly PORT_RANGE_INVALID: string = 'Port 范围应为 1-65535';
    static readonly VALIDATION_SEPARATOR: string = '；';
    static readonly SESSION_REMOTE_LOGIN_WAITING_TITLE: string = '已连接，等待远端桌面';
    static readonly SESSION_REMOTE_LOGIN_WAITING_SUBTITLE: string = '远端桌面登录阶段暂未收到新画面';
    static readonly SESSION_CONNECTED_TITLE: string = '已连接，正在打开远端桌面';
    static readonly SESSION_CONNECTED_SUBTITLE: string = '等待远端桌面首帧';
    static readonly SESSION_RESOLVING_TITLE: string = '正在解析主机';
    static readonly SESSION_RESOLVING_SUBTITLE: string = '准备建立 RDP 连接';
    static readonly SESSION_TCP_CONNECTED_TITLE: string = '已连通主机';
    static readonly SESSION_TCP_CONNECTED_SUBTITLE: string = '正在协商远程桌面会话';
    static readonly SESSION_NEGOTIATING_TITLE: string = '正在协商远程桌面';
    static readonly SESSION_NEGOTIATING_SUBTITLE: string = '等待服务端返回图形能力';
    static readonly SESSION_AUTHENTICATING_TITLE: string = '正在验证登录信息';
    static readonly SESSION_AUTHENTICATING_SUBTITLE: string = '即将进入远端桌面';
    static readonly SESSION_CONNECTING_TITLE: string = '正在连接远程桌面';
    static readonly SESSION_CONNECTING_SUBTITLE: string = '正在建立 RDP 会话';
    static connectionFailureWithDetail(detail: string): string {
        bjccovmshb1i7s.instrumentFunction(0);
        bjccovmshb1i7s.instrumentRegion(0, 1);
        return `${HomeText.CONNECTION_FAILURE}：${detail}`;
    }
}
