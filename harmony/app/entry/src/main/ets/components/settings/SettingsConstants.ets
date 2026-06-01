import { SettingsAppearanceMode, SettingsStatusTone } from './SettingsTypes'

export const REMOTE_FILES_SHARED_DIR_STORAGE_KEY: string = 'remoteFilesSharedDirPath'
export const REMOTE_FILES_SHARED_DIR_NAME: string = 'com.muhub.desktop'

export class SettingsRoute {
  static readonly SETTINGS: string = 'settings'
  static readonly BASIC: string = 'basic'
  static readonly REMOTE_CONTROL: string = 'remoteControl'
  static readonly PROJECT_HELP: string = 'projectHelp'
}

export class SettingsRemoteControlSection {
  static readonly SERVER: string = 'server'
  static readonly SCREEN: string = 'screen'
  static readonly ACCESS: string = 'access'
  static readonly FILES: string = 'files'
}

export class SettingsText {
  static readonly SETTINGS_TITLE: string = '设置'
  static readonly SETTINGS_OVERVIEW_TITLE: string = '设置概览'
  static readonly SETTINGS_OVERVIEW_SUBTITLE: string = '查看当前外观、被控服务、权限和共享目录状态'
  static readonly SETTINGS_CURRENT_STATE: string = '当前状态'
  static readonly SETTINGS_NAV_OVERVIEW: string = '概览'
  static readonly BASIC_SETTINGS_ENTRY_TITLE: string = '基础设置'
  static readonly BASIC_SETTINGS_ENTRY_DESC: string = '外观模式、本机 IP 和连接前辅助信息'
  static readonly REMOTE_CONTROL_ENTRY_TITLE: string = '远控设置'
  static readonly REMOTE_CONTROL_ENTRY_DESC: string = '管理主控访问门禁、视频流授权和被控端状态'
  static readonly PROJECT_HELP_ENTRY_TITLE: string = '项目帮助'
  static readonly PROJECT_HELP_ENTRY_DESC: string = '产品信息、许可说明、使用说明和常见排查'

  static readonly APPEARANCE_SECTION_MODE_DESC: string = '选择后立即应用，跟随系统会监听系统深浅色变化。'
  static readonly FOLLOW_SYSTEM_TITLE: string = '跟随系统'
  static readonly FOLLOW_SYSTEM_DESC: string = '读取系统深浅色设置，系统切换后应用外观随之变化。'
  static readonly LIGHT_MODE_TITLE: string = '浅色模式'
  static readonly LIGHT_MODE_DESC: string = '使用浅色界面。'
  static readonly DARK_MODE_TITLE: string = '深色模式'
  static readonly DARK_MODE_DESC: string = '使用深色界面。'

  static readonly BASIC_SETTINGS_TITLE: string = '基础设置'
  static readonly BASIC_SETTINGS_SUBTITLE: string = '调整界面外观，查看当前设备网络信息'
  static readonly BASIC_APPEARANCE_SECTION: string = '外观'
  static readonly BASIC_NETWORK_SECTION: string = '本机网络'

  static readonly REMOTE_CONTROL_TITLE: string = '远控设置'
  static readonly REMOTE_CONTROL_SUBTITLE: string = '管理当前设备作为被控端时的授权、门禁和端口状态'
  static readonly REMOTE_SERVER_TITLE: string = '被控端连接端口'
  static readonly REMOTE_SERVER_DESC: string = '主控端远程连接当前设备时使用该监听端口。'
  static readonly REMOTE_SERVER_STATUS_LABEL: string = '服务状态'
  static readonly REMOTE_SERVER_RUNNING: string = '已启动'
  static readonly REMOTE_SERVER_STOPPED: string = '未启动'
  static readonly REMOTE_SERVER_EXITED: string = '已退出'
  static readonly REMOTE_SERVER_FAILED: string = '启动失败'
  static readonly REMOTE_SERVER_BUSY: string = '处理中'
  static readonly REMOTE_SERVER_START_ACTION: string = '启动服务'
  static readonly REMOTE_SERVER_REFRESH_ACTION: string = '刷新'
  static readonly REMOTE_SERVER_MESSAGE_LISTENING: string = '被控服务正在监听主控端连接。'
  static readonly REMOTE_SERVER_MESSAGE_ACTIVE: string = '已有主控端远程会话连接。'
  static readonly REMOTE_SERVER_MESSAGE_STOPPED: string = '服务未启动，可点击启动服务。'
  static readonly REMOTE_SERVER_MESSAGE_EXITED: string = '服务已退出，可尝试重新启动。'
  static readonly REMOTE_SERVER_MESSAGE_FAILED: string = '服务状态读取失败。'
  static readonly REMOTE_ACCESS_SECTION: string = '验证码登录'
  static readonly REMOTE_ACCESS_GATE_TITLE: string = '主控连接验证码'
  static readonly REMOTE_ACCESS_GATE_DESC: string = '开启后，主控端连接当前设备需要输入当前验证码；切换后会重启被控服务生效。'
  static readonly REMOTE_ACCESS_CODE_LABEL: string = '当前验证码'
  static readonly REMOTE_ACCESS_CODE_DISABLED: string = '默认关闭'
  static readonly REMOTE_ACCESS_CODE_ACTION: string = '重新生成验证码'
  static readonly REMOTE_ACCESS_GATE_ON: string = '已开启'
  static readonly REMOTE_ACCESS_GATE_OFF: string = '默认关闭'
  static readonly REMOTE_SCREEN_SECTION: string = '录屏权限'
  static readonly REMOTE_SCREEN_PERMISSION_TITLE: string = '视频流录屏授权'
  static readonly REMOTE_SCREEN_PERMISSION_DESC: string = '授权后，主控端远程访问才能接收被控端桌面视频流。'
  static readonly REMOTE_SCREEN_PERMISSION_GRANTED: string = '已授权'
  static readonly REMOTE_SCREEN_PERMISSION_MISSING: string = '未授权'
  static readonly REMOTE_SCREEN_PERMISSION_ACTION: string = '去授权'
  static readonly REMOTE_SCREEN_PERMISSION_BUSY: string = '处理中'
  static readonly REMOTE_FILES_FEATURE_TITLE: string = '鸿蒙共享目录'
  static readonly REMOTE_FILES_FEATURE_DESC: string = '主控端连接后，可在远程桌面中通过 \\\\tsclient\\Downloads 访问当前设备共享目录。'
  static readonly REMOTE_FILES_FEATURE_ACTION: string = '共享的鸿蒙目录'

  static readonly PROJECT_HELP_TITLE: string = '项目帮助'
  static readonly PROJECT_HELP_SUBTITLE: string = '产品信息、许可说明和使用帮助'
  static readonly PROJECT_HELP_ABOUT_SECTION: string = '关于项目'

  static readonly USAGE_SECTION_PREP: string = '连接流程'
  static readonly USAGE_SECTION_PREP_DESC: string = '先确认目标机和网络，再填写连接信息。'
  static readonly USAGE_SECTION_SECURITY: string = '安全与排查'
  static readonly USAGE_SECTION_SECURITY_DESC: string = '证书策略和常见连接失败原因。'
  static readonly USAGE_PREP_TITLE: string = '连接前准备'
  static readonly USAGE_PREP_BODY: string =
    '确认目标 Windows 电脑已经开启远程桌面服务，并且当前设备能访问目标机的 3389 端口。目标账号需要允许远程登录，且不能使用空密码。'
  static readonly USAGE_FORM_TITLE: string = '填写连接信息'
  static readonly USAGE_FORM_BODY: string =
    '在主界面填写 Windows host、Port、Username 和 Password。host 可以是目标电脑 IP 或主机名，默认端口是 3389。'
static readonly USAGE_CERT_TITLE: string = '证书策略'
  static readonly USAGE_CERT_BODY: string =
    '当前版本暂时固定使用 TOFU 证书策略，适合测试和内网首次连接，会在首次连接时信任目标证书。后续需要更严格校验时再开放策略选择入口。'
  static readonly USAGE_WINDOWS_RDP_TITLE: string = 'Windows 远程桌面开关'
  static readonly USAGE_WINDOWS_RDP_BODY: string =
    '连接失败时，先在目标 Windows 的“设置 > 系统 > 远程桌面”确认远程桌面已开启，并允许当前账号远程登录。用户名建议按“设备名\\登录用户名”填写；首次开启远程桌面后，建议重启 Windows 再连接，避免服务和防火墙规则未即时生效。'
  static readonly USAGE_TROUBLESHOOT_TITLE: string = '常见排查'
  static readonly USAGE_TROUBLESHOOT_BODY: string =
    '连不上时优先确认本机 IP、目标 IP、同一网络、Windows 防火墙、远程桌面开关、账号权限和端口 3389 是否可达。'

  static readonly NETWORK_SECTION_DETAIL_DESC: string = '默认网络接口和当前可用 IP 地址。'
  static readonly NETWORK_NAME_LABEL: string = '当前网络'
  static readonly NETWORK_IP_LABEL: string = 'IP 地址'
  static readonly NETWORK_REFRESH_ACTION: string = '刷新'
  static readonly NETWORK_LOADING: string = '查询中'
  static readonly NETWORK_EMPTY: string = '未连接网络'
  static readonly NETWORK_NO_IP: string = '未获取到可用 IP'
  static readonly NETWORK_READ_FAILED: string = '无法读取本机 IP'
  static readonly EMPTY_VALUE: string = '-'

  static readonly ABOUT_SECTION_PROJECT_DESC: string = '查看产品信息、组件适配和随项目提供的许可说明。'
  static readonly ABOUT_GITHUB_LABEL: string = '项目主页'
  static readonly ABOUT_GITHUB_VALUE: string = 'https://github.com/xiaomu120413/freerdp-control-demo'
  static readonly ABOUT_FREERDP_ADAPTATION_LABEL: string = 'FreeRDP 鸿蒙化地址'
  static readonly ABOUT_FREERDP_ADAPTATION_VALUE: string = 'https://github.com/xiaomu120413/FreeRDP/tree/ohos-port'
  static readonly ABOUT_XRDP_ADAPTATION_LABEL: string = 'xrdp 鸿蒙化地址'
  static readonly ABOUT_XRDP_ADAPTATION_VALUE: string = 'https://github.com/xiaomu120413/xrdp/tree/ohos-port'
  static readonly ABOUT_LICENSE_LABEL: string = '许可信息'
  static readonly ABOUT_LICENSE_VALUE: string = 'MIT license'

  static appearanceValue(mode: SettingsAppearanceMode): string {
    if (mode === 'dark') {
      return '深色模式'
    }
    if (mode === 'light') {
      return '浅色模式'
    }
    return '跟随系统'
  }

  static remoteServerStatusTone(running: boolean, busy: boolean, state: string): SettingsStatusTone {
    if (busy) {
      return 'info'
    }
    if (state === 'Failed' || state === 'Exited') {
      return 'danger'
    }
    if (running) {
      return 'ok'
    }
    return 'neutral'
  }

  static remoteServerStateLabel(running: boolean, state: string): string {
    if (state === 'Failed') {
      return SettingsText.REMOTE_SERVER_FAILED
    }
    if (state === 'Exited') {
      return SettingsText.REMOTE_SERVER_EXITED
    }
    return running ? SettingsText.REMOTE_SERVER_RUNNING : SettingsText.REMOTE_SERVER_STOPPED
  }

  static remoteServerStatusText(running: boolean, busy: boolean, state: string, port: number): string {
    if (busy) {
      return SettingsText.REMOTE_SERVER_BUSY
    }
    const safePort = port > 0 ? port : 3390
    return `${SettingsText.remoteServerStateLabel(running, state)} / ${safePort}`
  }

}
