import type { SettingsAppearanceMode, SettingsStatusTone } from './SettingsTypes';
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
let bjccovmshb1iai = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "common/src/main/ets/components/settings/SettingsConstants.ets", hash: "42b4a6e34a81f110bfee0e59024b65fe30df767bfdf31306dd2072b8e70ec2c3", lineCnt: 179, count: 0, projectPath: "", functions: { 0: { name: "SettingsText.appearanceValue", count: 0, regions: { 0: { startLoc: { line: 137, col: 3 }, endLoc: { line: 145, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 138, col: 26 }, endLoc: { line: 140, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 141, col: 27 }, endLoc: { line: 143, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 144, col: 5 }, endLoc: { line: 145, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 138, col: 9 }, endLoc: { line: 138, col: 24 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 141, col: 9 }, endLoc: { line: 141, col: 25 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 }, 1: { name: "SettingsText.remoteServerStatusTone", count: 0, regions: { 0: { startLoc: { line: 147, col: 3 }, endLoc: { line: 158, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 148, col: 15 }, endLoc: { line: 150, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 151, col: 51 }, endLoc: { line: 153, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 154, col: 18 }, endLoc: { line: 156, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 157, col: 5 }, endLoc: { line: 158, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 148, col: 9 }, endLoc: { line: 148, col: 13 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 151, col: 9 }, endLoc: { line: 151, col: 49 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 154, col: 9 }, endLoc: { line: 154, col: 16 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 1 }, 2: { name: "SettingsText.remoteServerStateLabel", count: 0, regions: { 0: { startLoc: { line: 160, col: 3 }, endLoc: { line: 168, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 161, col: 29 }, endLoc: { line: 163, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 164, col: 29 }, endLoc: { line: 166, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 167, col: 5 }, endLoc: { line: 168, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 161, col: 9 }, endLoc: { line: 161, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 164, col: 9 }, endLoc: { line: 164, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 167, col: 12 }, endLoc: { line: 167, col: 93 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 2 }, 3: { name: "SettingsText.remoteServerStatusText", count: 0, regions: { 0: { startLoc: { line: 170, col: 3 }, endLoc: { line: 176, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 171, col: 15 }, endLoc: { line: 173, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 174, col: 5 }, endLoc: { line: 176, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 171, col: 9 }, endLoc: { line: 171, col: 13 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 174, col: 22 }, endLoc: { line: 174, col: 44 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 3 } }, exeLine: { 0: 1, 1: 3, 2: 4, 3: 6, 4: 7, 5: 8, 6: 9, 7: 10, 8: 13, 9: 14, 10: 15, 11: 16, 12: 17, 13: 18, 14: 21, 15: 22, 16: 23, 17: 24, 18: 25, 19: 26, 20: 27, 21: 28, 22: 29, 23: 30, 24: 31, 25: 32, 26: 33, 27: 35, 28: 36, 29: 37, 30: 38, 31: 39, 32: 40, 33: 41, 34: 43, 35: 44, 36: 45, 37: 46, 38: 48, 39: 49, 40: 50, 41: 51, 42: 52, 43: 53, 44: 54, 45: 55, 46: 56, 47: 57, 48: 58, 49: 59, 50: 60, 51: 61, 52: 62, 53: 63, 54: 64, 55: 65, 56: 66, 57: 67, 58: 68, 59: 69, 60: 70, 61: 71, 62: 72, 63: 73, 64: 74, 65: 75, 66: 76, 67: 77, 68: 78, 69: 79, 70: 80, 71: 81, 72: 82, 73: 83, 74: 84, 75: 85, 76: 87, 77: 88, 78: 89, 79: 91, 80: 92, 81: 93, 82: 94, 83: 95, 84: 96, 85: 97, 86: 98, 87: 99, 88: 100, 89: 101, 90: 102, 91: 103, 92: 104, 93: 105, 94: 106, 95: 107, 96: 108, 97: 109, 98: 110, 99: 111, 100: 112, 101: 113, 102: 114, 103: 115, 104: 117, 105: 118, 106: 119, 107: 120, 108: 121, 109: 122, 110: 123, 111: 124, 112: 125, 113: 127, 114: 128, 115: 129, 116: 130, 117: 131, 118: 132, 119: 133, 120: 134, 121: 135, 122: 137, 123: 138, 124: 139, 125: 141, 126: 142, 127: 144, 128: 147, 129: 148, 130: 149, 131: 151, 132: 152, 133: 154, 134: 155, 135: 157, 136: 160, 137: 161, 138: 162, 139: 164, 140: 165, 141: 167, 142: 170, 143: 171, 144: 172, 145: 174, 146: 175 } });
export const REMOTE_FILES_SHARED_DIR_STORAGE_KEY: string = 'remoteFilesSharedDirPath';
export const REMOTE_FILES_SHARED_DIR_NAME: string = 'com.muhub.desktop';
export class SettingsRoute {
    static readonly SETTINGS: string = 'settings';
    static readonly BASIC: string = 'basic';
    static readonly REMOTE_CONTROL: string = 'remoteControl';
    static readonly PROJECT_HELP: string = 'projectHelp';
}
export class SettingsRemoteControlSection {
    static readonly SERVER: string = 'server';
    static readonly SCREEN: string = 'screen';
    static readonly INPUT: string = 'input';
    static readonly ACCESS: string = 'access';
    static readonly FILES: string = 'files';
}
export class SettingsText {
    static readonly SETTINGS_TITLE: string = '设置';
    static readonly SETTINGS_OVERVIEW_TITLE: string = '设置概览';
    static readonly SETTINGS_OVERVIEW_SUBTITLE: string = '查看当前外观、被控服务、权限和共享目录状态';
    static readonly SETTINGS_OVERVIEW_CLIENT_SUBTITLE: string = '查看当前外观、连接辅助信息和共享目录';
    static readonly SETTINGS_CURRENT_STATE: string = '当前状态';
    static readonly SETTINGS_NAV_OVERVIEW: string = '概览';
    static readonly BASIC_SETTINGS_ENTRY_TITLE: string = '基础设置';
    static readonly BASIC_SETTINGS_ENTRY_DESC: string = '外观模式、本机 IP 和连接前辅助信息';
    static readonly REMOTE_CONTROL_ENTRY_TITLE: string = '远控设置';
    static readonly REMOTE_CONTROL_ENTRY_DESC: string = '管理主控访问门禁、视频流授权和被控端状态';
    static readonly PROJECT_HELP_ENTRY_TITLE: string = '项目帮助';
    static readonly PROJECT_HELP_ENTRY_DESC: string = '产品信息、许可说明、使用说明和常见排查';
    static readonly APPEARANCE_SECTION_MODE_DESC: string = '选择后立即应用，跟随系统会监听系统深浅色变化。';
    static readonly FOLLOW_SYSTEM_TITLE: string = '跟随系统';
    static readonly FOLLOW_SYSTEM_DESC: string = '读取系统深浅色设置，系统切换后应用外观随之变化。';
    static readonly LIGHT_MODE_TITLE: string = '浅色模式';
    static readonly LIGHT_MODE_DESC: string = '使用浅色界面。';
    static readonly DARK_MODE_TITLE: string = '深色模式';
    static readonly DARK_MODE_DESC: string = '使用深色界面。';
    static readonly BASIC_SETTINGS_TITLE: string = '基础设置';
    static readonly BASIC_SETTINGS_SUBTITLE: string = '调整界面外观，查看当前设备网络信息';
    static readonly BASIC_APPEARANCE_SECTION: string = '外观';
    static readonly BASIC_NETWORK_SECTION: string = '本机网络';
    static readonly REMOTE_CONTROL_TITLE: string = '远控设置';
    static readonly REMOTE_CONTROL_SUBTITLE: string = '管理当前设备作为被控端时的授权、门禁和端口状态';
    static readonly REMOTE_SERVER_TITLE: string = '被控端连接端口';
    static readonly REMOTE_SERVER_DESC: string = '主控端远程连接当前设备时使用该监听端口。';
    static readonly REMOTE_SERVER_STATUS_LABEL: string = '服务状态';
    static readonly REMOTE_SERVER_RUNNING: string = '已启动';
    static readonly REMOTE_SERVER_STOPPED: string = '未启动';
    static readonly REMOTE_SERVER_EXITED: string = '已退出';
    static readonly REMOTE_SERVER_FAILED: string = '启动失败';
    static readonly REMOTE_SERVER_BUSY: string = '启动中';
    static readonly REMOTE_SERVER_START_ACTION: string = '启动服务';
    static readonly REMOTE_SERVER_REFRESH_ACTION: string = '刷新';
    static readonly REMOTE_SERVER_MESSAGE_LISTENING: string = '被控服务正在监听主控端连接。';
    static readonly REMOTE_SERVER_MESSAGE_ACTIVE: string = '已有主控端远程会话连接。';
    static readonly REMOTE_SERVER_MESSAGE_STOPPED: string = '服务未启动，可点击启动服务。';
    static readonly REMOTE_SERVER_MESSAGE_EXITED: string = '服务已退出，可尝试重新启动。';
    static readonly REMOTE_SERVER_MESSAGE_FAILED: string = '服务状态读取失败。';
    static readonly REMOTE_SERVER_MESSAGE_UNAVAILABLE: string = '当前设备仅支持作为主控端。';
    static readonly REMOTE_ACCESS_SECTION: string = '验证码登录';
    static readonly REMOTE_ACCESS_GATE_TITLE: string = '主控连接验证码';
    static readonly REMOTE_ACCESS_GATE_DESC: string = '开启后，主控端连接当前设备需要输入当前验证码；切换后会重启被控服务生效。';
    static readonly REMOTE_ACCESS_CODE_LABEL: string = '当前验证码';
    static readonly REMOTE_ACCESS_CODE_DISABLED: string = '默认关闭';
    static readonly REMOTE_ACCESS_CODE_ACTION: string = '重新生成验证码';
    static readonly REMOTE_ACCESS_GATE_ON: string = '已开启';
    static readonly REMOTE_ACCESS_GATE_OFF: string = '默认关闭';
    static readonly REMOTE_SCREEN_SECTION: string = '录屏权限';
    static readonly REMOTE_SCREEN_PERMISSION_TITLE: string = '视频流录屏授权';
    static readonly REMOTE_SCREEN_PERMISSION_DESC: string = '授权后，主控端远程访问才能接收被控端桌面视频流。';
    static readonly REMOTE_INPUT_PERMISSION_TITLE: string = '键鼠输入注入授权';
    static readonly REMOTE_INPUT_PERMISSION_DESC: string = '授权后，主控端可长期向当前设备注入键盘和鼠标操作。';
    static readonly REMOTE_PERMISSION_GRANTED: string = '已授权';
    static readonly REMOTE_PERMISSION_MISSING: string = '未授权';
    static readonly REMOTE_PERMISSION_ACTION: string = '去授权';
    static readonly REMOTE_PERMISSION_BUSY: string = '授权中';
    static readonly REMOTE_FILES_FEATURE_TITLE: string = '鸿蒙共享目录';
    static readonly REMOTE_FILES_FEATURE_DESC: string = '主控端连接后，可在远程桌面中通过 \\\\tsclient\\Downloads 访问当前设备共享目录。';
    static readonly REMOTE_FILES_FEATURE_ACTION: string = '共享的鸿蒙目录';
    static readonly PROJECT_HELP_TITLE: string = '项目帮助';
    static readonly PROJECT_HELP_SUBTITLE: string = '产品信息、许可说明和使用帮助';
    static readonly PROJECT_HELP_ABOUT_SECTION: string = '关于项目';
    static readonly USAGE_SECTION_PREP: string = '连接流程';
    static readonly USAGE_SECTION_PREP_DESC: string = '按顺序完成 Windows 设置、账号确认和连接信息填写。';
    static readonly USAGE_SECTION_SECURITY: string = '安全与排查';
    static readonly USAGE_SECTION_SECURITY_DESC: string = '证书策略和常见连接失败原因。';
    static readonly USAGE_PREP_TITLE: string = '连接前准备';
    static readonly USAGE_PREP_BODY: string = '确认当前设备和 Windows 电脑网络互通，并能访问 Windows 的远程桌面端口（默认 3389）。远程登录账号需要设置非空密码。';
    static readonly USAGE_FORM_TITLE: string = '4. 填写连接信息';
    static readonly USAGE_FORM_BODY: string = 'Host 填 Windows 电脑的 IP 或设备名，Port 通常为 3389，Username 按上一步的账号格式填写，Password 填该 Windows 账号密码（不要填 Windows Hello PIN）。';
    static readonly USAGE_CERT_TITLE: string = '证书策略';
    static readonly USAGE_CERT_BODY: string = '当前版本暂时固定使用 TOFU 证书策略，适合测试和内网首次连接，会在首次连接时信任目标证书。后续需要更严格校验时再开放策略选择入口。';
    static readonly USAGE_WINDOWS_RDP_TITLE: string = '1. 开启 Windows 远程桌面';
    static readonly USAGE_WINDOWS_RDP_BODY: string = '在被连接的 Windows 电脑打开“设置 > 系统 > 远程桌面”，开启“远程桌面”，并确认防火墙允许远程桌面、当前账号允许远程登录。首次开启后建议重启 Windows。';
    static readonly USAGE_USERNAME_TITLE: string = '2. 确认 Windows 用户名';
    static readonly USAGE_USERNAME_BODY: string = '不要填写显示昵称。本地账号填“设备名\\用户名”，域账号填“域名\\用户名”，微软账号填“MicrosoftAccount\\登录邮箱”。设备名可在“设置 > 系统 > 系统信息”查看；本地或域账号可在命令提示符运行 whoami 核对。';
    static readonly USAGE_HARDWARE_ACCEL_TITLE: string = '3. 开启 Windows 视频硬件加速（可选）';
    static readonly USAGE_HARDWARE_ACCEL_BODY: string = 'Windows 专业版/企业版/教育版：按 Win+R，输入 gpedit.msc，进入“计算机配置 > 管理模板 > Windows 组件 > 远程桌面服务 > 远程桌面会话主机 > 远程会话环境”，启用“为所有远程桌面服务会话使用硬件图形适配器”“配置远程桌面连接的 H.264/AVC 硬件编码”和“为远程桌面连接优先使用 H.264/AVC 444 图形模式”，然后重启 Windows。家庭版通常没有本地组策略编辑器；显卡或驱动不支持时会回退软件编码。';
    static readonly USAGE_TROUBLESHOOT_TITLE: string = '常见排查';
    static readonly USAGE_TROUBLESHOOT_BODY: string = '连不上时优先确认本机 IP、目标 IP、同一网络、Windows 防火墙、远程桌面开关、账号权限和端口 3389 是否可达。';
    static readonly NETWORK_SECTION_DETAIL_DESC: string = '默认网络接口和当前可用 IP 地址。';
    static readonly NETWORK_NAME_LABEL: string = '当前网络';
    static readonly NETWORK_IP_LABEL: string = 'IP 地址';
    static readonly NETWORK_REFRESH_ACTION: string = '刷新';
    static readonly NETWORK_LOADING: string = '查询中';
    static readonly NETWORK_EMPTY: string = '未连接网络';
    static readonly NETWORK_NO_IP: string = '未获取到可用 IP';
    static readonly NETWORK_READ_FAILED: string = '无法读取本机 IP';
    static readonly EMPTY_VALUE: string = '-';
    static readonly ABOUT_SECTION_PROJECT_DESC: string = '查看产品信息、组件适配和随项目提供的许可说明。';
    static readonly ABOUT_GITHUB_LABEL: string = '项目主页';
    static readonly ABOUT_GITHUB_VALUE: string = 'https://github.com/xiaomu120413/freerdp-control-demo';
    static readonly ABOUT_FREERDP_ADAPTATION_LABEL: string = 'FreeRDP 鸿蒙化地址';
    static readonly ABOUT_FREERDP_ADAPTATION_VALUE: string = 'https://github.com/xiaomu120413/FreeRDP/tree/ohos-port';
    static readonly ABOUT_XRDP_ADAPTATION_LABEL: string = 'xrdp 鸿蒙化地址';
    static readonly ABOUT_XRDP_ADAPTATION_VALUE: string = 'https://github.com/xiaomu120413/xrdp/tree/ohos-port';
    static readonly ABOUT_LICENSE_LABEL: string = '许可信息';
    static readonly ABOUT_LICENSE_VALUE: string = 'MIT license';
    static appearanceValue(mode: SettingsAppearanceMode): string {
        bjccovmshb1iai.instrumentFunction(0);
        if (mode === 'dark') {
            bjccovmshb1iai.instrumentBranch(0, 0, true);
            bjccovmshb1iai.instrumentRegion(0, 1);
            return '深色模式';
        }
        else {
            bjccovmshb1iai.instrumentBranch(0, 0, false);
        }
        if (mode === 'light') {
            bjccovmshb1iai.instrumentBranch(0, 1, true);
            bjccovmshb1iai.instrumentRegion(0, 2);
            return '浅色模式';
        }
        else {
            bjccovmshb1iai.instrumentBranch(0, 1, false);
        }
        bjccovmshb1iai.instrumentRegion(0, 3);
        return '跟随系统';
    }
    static remoteServerStatusTone(running: boolean, busy: boolean, state: string): SettingsStatusTone {
        bjccovmshb1iai.instrumentFunction(1);
        if (busy) {
            bjccovmshb1iai.instrumentBranch(1, 0, true);
            bjccovmshb1iai.instrumentRegion(1, 1);
            return 'info';
        }
        else {
            bjccovmshb1iai.instrumentBranch(1, 0, false);
        }
        if (state === 'Failed' || state === 'Exited') {
            bjccovmshb1iai.instrumentBranch(1, 1, true);
            bjccovmshb1iai.instrumentRegion(1, 2);
            return 'danger';
        }
        else {
            bjccovmshb1iai.instrumentBranch(1, 1, false);
        }
        if (running) {
            bjccovmshb1iai.instrumentBranch(1, 2, true);
            bjccovmshb1iai.instrumentRegion(1, 3);
            return 'ok';
        }
        else {
            bjccovmshb1iai.instrumentBranch(1, 2, false);
        }
        bjccovmshb1iai.instrumentRegion(1, 4);
        return 'neutral';
    }
    static remoteServerStateLabel(running: boolean, state: string): string {
        bjccovmshb1iai.instrumentFunction(2);
        if (state === 'Failed') {
            bjccovmshb1iai.instrumentBranch(2, 0, true);
            bjccovmshb1iai.instrumentRegion(2, 1);
            return SettingsText.REMOTE_SERVER_FAILED;
        }
        else {
            bjccovmshb1iai.instrumentBranch(2, 0, false);
        }
        if (state === 'Exited') {
            bjccovmshb1iai.instrumentBranch(2, 1, true);
            bjccovmshb1iai.instrumentRegion(2, 2);
            return SettingsText.REMOTE_SERVER_EXITED;
        }
        else {
            bjccovmshb1iai.instrumentBranch(2, 1, false);
        }
        bjccovmshb1iai.instrumentRegion(2, 3);
        return running ? (bjccovmshb1iai.instrumentBranch(2, 2, true), SettingsText.REMOTE_SERVER_RUNNING) : (bjccovmshb1iai.instrumentBranch(2, 2, false), SettingsText.REMOTE_SERVER_STOPPED);
    }
    static remoteServerStatusText(running: boolean, busy: boolean, state: string, port: number): string {
        bjccovmshb1iai.instrumentFunction(3);
        if (busy) {
            bjccovmshb1iai.instrumentBranch(3, 0, true);
            bjccovmshb1iai.instrumentRegion(3, 1);
            return SettingsText.REMOTE_SERVER_BUSY;
        }
        else {
            bjccovmshb1iai.instrumentBranch(3, 0, false);
        }
        bjccovmshb1iai.instrumentRegion(3, 2);
        const safePort = port > 0 ? (bjccovmshb1iai.instrumentBranch(3, 1, true), port) : (bjccovmshb1iai.instrumentBranch(3, 1, false), 3390);
        return `${SettingsText.remoteServerStateLabel(running, state)} / ${safePort}`;
    }
}
