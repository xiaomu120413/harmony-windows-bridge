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
  static readonly INPUT: string = 'input'
  static readonly ACCESS: string = 'access'
  static readonly FILES: string = 'files'
  static readonly DIAGNOSTICS: string = 'diagnostics'
}

// Settings shell and navigation copy shared by page titles and overview entries.
export class SettingsShellText {
  static readonly TITLE: string = '设置'
  static readonly OVERVIEW_TITLE: string = '设置概览'
  static readonly OVERVIEW_SUBTITLE: string = '本机作为被控端状态卡片'
  static readonly OVERVIEW_CLIENT_SUBTITLE: string = '查看当前外观、连接辅助信息和共享目录'
  static readonly CURRENT_STATE: string = '当前状态'
  static readonly BASIC_TITLE: string = '基础设置'
  static readonly BASIC_DESC: string = '调整应用外观与通用偏好'
  static readonly REMOTE_TITLE: string = '远控设置'
  static readonly REMOTE_DESC: string = '管理本机的被动控制与主动控制能力'
  static readonly REMOTE_CLIENT_DESC: string = '管理本机主动连接 Windows 的能力'
  static readonly HELP_TITLE: string = '项目帮助'
  static readonly HELP_DESC: string = '查看windows 如何连接本机，以及本机如何连接 Windows'
  static readonly HELP_CLIENT_DESC: string = '查看本机连接 Windows 的步骤和排查方法'
}

export class SettingsAppearanceText {
  static readonly SECTION: string = '外观'
  static readonly SECTION_DESC: string = '设置应用的深浅色模式。'
  static readonly FOLLOW_SYSTEM: string = '跟随系统'
  static readonly FOLLOW_SYSTEM_DESC: string = '读取系统深浅模式，系统切换后应用跟随。'
  static readonly LIGHT: string = '浅色模式'
  static readonly LIGHT_DESC: string = '使用浅色模式。'
  static readonly DARK: string = '深色模式'
  static readonly DARK_DESC: string = '使用深色模式。'

  static value(mode: SettingsAppearanceMode): string {
    if (mode === 'dark') {
      return SettingsAppearanceText.DARK
    }
    if (mode === 'light') {
      return SettingsAppearanceText.LIGHT
    }
    return SettingsAppearanceText.FOLLOW_SYSTEM
  }
}

export class SettingsRemoteText {
  static readonly PAGE_SUBTITLE: string = '管理别人连接本机和本机主动连接 Windows 的能力'
  static readonly PAGE_CONTROLLER_SUBTITLE: string = '管理本机主动连接 Windows 的共享目录和连接验证码'
  static readonly PASSIVE_SECTION: string = '被动控制'
  static readonly PASSIVE_DESC: string = 'windows机器 连接并控制本机。'
  static readonly ACTIVE_SECTION: string = '主动控制'
  static readonly ACTIVE_DESC: string = '本机连接并控制 Windows机器。'

  static readonly CONTROLLED_DEVICE_TITLE: string = '让windows 机器连接这台设备'
  static readonly SERVER_DESC: string = '服务启动后，可通过mstsc连接本机。'
  static readonly SERVER_STATUS_LABEL: string = '服务状态'
  static readonly SERVER_RUNNING: string = '已启动'
  static readonly SERVER_STOPPED: string = '未启动'
  static readonly SERVER_EXITED: string = '已退出'
  static readonly SERVER_FAILED: string = '启动失败'
  static readonly SERVER_BUSY: string = '启动中'
  static readonly SERVER_START_ACTION: string = '启动服务'
  static readonly SERVER_REFRESH_ACTION: string = '刷新'
  static readonly SERVER_STOP_ACTION: string = '停止服务'
  static readonly SERVER_MESSAGE_LISTENING: string = '被控服务正在监听主控端连接。'
  static readonly SERVER_MESSAGE_ACTIVE: string = '已有主控端远程会话连接。'
  static readonly SERVER_MESSAGE_STOPPED: string = '服务未启动，可点击启动服务。'
  static readonly SERVER_MESSAGE_EXITED: string = '服务已退出，可尝试重新启动。'
  static readonly SERVER_MESSAGE_FAILED: string = '服务状态读取失败。'
  static readonly SERVER_MESSAGE_UNAVAILABLE: string = '当前设备仅支持作为主控端。'

  static readonly CONNECTION_ADDRESS_LABEL: string = '连接地址'
  static readonly CONNECTION_ADDRESS_WAITING: string = '正在获取本机 IP'
  static readonly CONNECTION_NETWORK_HINT: string = '两台设备需位于同一网络，或所在网络之间可以互相访问。'

  static readonly ACCESS_TITLE: string = '主动连接验证码'
  static readonly ACCESS_DESC: string = '本机作为主控端主动连接 Windows 时使用；由主控端设置并在连接时填写。'
  static readonly ACCESS_CODE_LABEL: string = '当前验证码'
  static readonly ACCESS_CODE_ACTION: string = '重新生成验证码'
  static readonly ACCESS_GATE_ON: string = '已开启'
  static readonly ACCESS_GATE_OFF: string = '默认关闭'

  static readonly SCREEN_SECTION: string = '录屏权限'
  static readonly SCREEN_PERMISSION_TITLE: string = '视频流录屏授权'
  static readonly SCREEN_PERMISSION_DESC: string = '授权后，主控端远程访问才能接收被控端桌面视频流。'
  static readonly INPUT_SECTION: string = '键鼠权限'
  static readonly INPUT_PERMISSION_TITLE: string = '键鼠输入注入授权'
  static readonly INPUT_PERMISSION_DESC: string = '授权后，主控端可长期向当前设备注入键盘和鼠标操作。'
  static readonly PERMISSION_GRANTED: string = '已授权'
  static readonly PERMISSION_MISSING: string = '未授权'
  static readonly PERMISSION_ACTION: string = '去授权'
  static readonly PERMISSION_BUSY: string = '授权中'

  static readonly FILES_TITLE: string = '鸿蒙共享目录'
  static readonly FILES_DESC: string =
    '本机作为主控端连接 Windows 后，可在远程桌面中通过 \\\\tsclient\\Downloads 访问本机共享目录。'
  static readonly FILES_ACTION: string = '共享的鸿蒙目录'
  static readonly DIAGNOSTICS_ACTION: string = '导出诊断'
  static readonly DIAGNOSTICS_EXPORTING: string = '导出中'
  static readonly DIAGNOSTICS_SECTION: string = '诊断与排障'
  static readonly DIAGNOSTICS_SECTION_DESC: string = '导出脱敏的连接、渲染、通道、权限和服务状态。'
  static readonly DIAGNOSTICS_TITLE: string = '一键导出诊断'
  static readonly DIAGNOSTICS_DESC: string =
    '包含连接阶段、端点探测、编码路径、帧率、丢帧、音频、resize、通道、权限和错误摘要；不包含密码、访问码或完整主机与用户名。'

  static serverStatusTone(running: boolean, busy: boolean, state: string): SettingsStatusTone {
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

  static serverStateLabel(running: boolean, state: string): string {
    if (state === 'Failed') {
      return SettingsRemoteText.SERVER_FAILED
    }
    if (state === 'Exited') {
      return SettingsRemoteText.SERVER_EXITED
    }
    return running ? SettingsRemoteText.SERVER_RUNNING : SettingsRemoteText.SERVER_STOPPED
  }

  static serverStatusText(running: boolean, busy: boolean, state: string, port: number): string {
    if (busy) {
      return SettingsRemoteText.SERVER_BUSY
    }
    const safePort = port > 0 ? port : 3390
    return `${SettingsRemoteText.serverStateLabel(running, state)} / ${safePort}`
  }
}

export class SettingsHelpText {
  static readonly PAGE_SUBTITLE: string = '按使用方向查看连接步骤和排查方法'
  static readonly ABOUT_SECTION: string = '关于项目'
  static readonly CONTROLLED_DESC: string = '本机作为被控端，对方使用 Windows 远程桌面连接。'
  static readonly CONTROLLED_READY: string = '被控服务已启动'
  static readonly CONTROLLED_NOT_READY: string = '被控服务未启动'
  static readonly CONTROLLED_READY_DESC: string = '本机已准备好，正在等待远程连接。'
  static readonly CONTROLLED_NOT_READY_DESC: string = '请先前往远控设置启动服务并完成授权。'
  static readonly CONTROLLED_SETTINGS_ACTION: string = '前往远控设置'
  static readonly CONTROLLED_STEPS_TITLE: string = '连接步骤'
  static readonly CONTROLLED_STEP_ONE: string = '本机保持服务运行'
  static readonly CONTROLLED_STEP_ONE_DESC: string = '在远控设置中启动被控服务，并完成录屏与键鼠授权。'
  static readonly CONTROLLED_STEP_TWO: string = '对方打开远程桌面连接'
  static readonly CONTROLLED_STEP_TWO_DESC: string = '在 Windows 按 Win+R，输入 mstsc 后回车。'
  static readonly CONTROLLED_STEP_THREE: string = '输入地址并连接'
  static readonly CONTROLLED_STEP_THREE_DESC: string = '在“计算机”中输入被控端连接地址，然后点击“连接”。'
  static readonly CONTROLLED_TROUBLESHOOT: string =
    '主控端：确认远程桌面已打开，连接地址、账号、密码等填写正确。\n被控端：确认被控服务正在运行，录屏与键鼠权限已授权。'
  static readonly CONTROLLER_TITLE: string = '我想用本机连接 Windows'
  static readonly CONTROLLER_DESC: string = '本机作为主控端，远程连接并控制另一台 Windows 电脑。'
}

export class SettingsUsageText {
  static readonly PREP_SECTION: string = '连接流程'
  static readonly PREP_SECTION_DESC: string = '按顺序完成 Windows 设置、账号确认和连接信息填写。'
  static readonly SECURITY_SECTION: string = '安全与排查'
  static readonly SECURITY_SECTION_DESC: string = '证书策略和常见连接失败原因。'
  static readonly PREP_TITLE: string = '连接前准备'
  static readonly PREP_BODY: string =
    '确认当前设备和 Windows 电脑网络互通，并能访问 Windows 的远程桌面端口（默认 3389）。远程登录账号需要设置非空密码。'
  static readonly FORM_TITLE: string = '4. 填写连接信息'
  static readonly FORM_BODY: string =
    'Host 填 Windows 电脑的 IP 或设备名，Port 通常为 3389，Username 按上一步的账号格式填写，Password 填该 Windows 账号密码。'
  static readonly CERT_TITLE: string = '证书策略'
  static readonly CERT_BODY: string =
    '当前版本暂时固定使用 TOFU 证书策略，适合测试和内网首次连接，会在首次连接时信任目标证书。后续需要更严格校验时再开放策略选择入口。'
  static readonly WINDOWS_RDP_TITLE: string = '1. 开启 Windows 远程桌面'
  static readonly WINDOWS_RDP_BODY: string =
    '在被连接的 Windows 电脑打开“设置 > 系统 > 远程桌面”，开启“远程桌面”，并确认防火墙允许远程桌面、当前账号允许远程登录。首次开启后建议重启 Windows。'
  static readonly USERNAME_TITLE: string = '2. 确认 Windows 用户名'
  static readonly USERNAME_BODY: string =
    '不要填写显示昵称。本地账号填“设备名\\用户名”，域账号填“域名\\用户名”，微软账号填“MicrosoftAccount\\登录邮箱”。设备名可在“设置 > 系统 > 系统信息”查看；本地或域账号可在命令提示符运行 whoami 核对。'
  static readonly HARDWARE_ACCEL_TITLE: string = '3. 开启 Windows 视频硬件加速（可选）'
  static readonly HARDWARE_ACCEL_BODY: string =
    'Windows主机按 Win+R，输入 gpedit.msc，进入“计算机配置 > 管理模板 > Windows 组件 > 远程桌面服务 > 远程桌面会话主机 > 远程会话环境”，启用“为所有远程桌面服务会话使用硬件图形适配器”“配置远程桌面连接的 H.264/AVC 硬件编码”和“为远程桌面连接优先使用 H.264/AVC 444 图形模式”，然后重启 Windows。家庭版通常没有本地组策略编辑器；显卡或驱动不支持时会回退软件编码。'
  static readonly TROUBLESHOOT_TITLE: string = '常见排查'
  static readonly TROUBLESHOOT_BODY: string =
    '连不上时优先确认本机 IP、目标 IP、同一网络、Windows 防火墙、远程桌面开关、账号权限和端口 3389 是否可达。'
}

export class SettingsNetworkText {
  static readonly LOADING: string = '查询中'
  static readonly EMPTY: string = '未连接网络'
  static readonly NO_IP: string = '未获取到可用 IP'
  static readonly READ_FAILED: string = '无法读取本机 IP'
  static readonly EMPTY_VALUE: string = '-'
}

export class SettingsAboutText {
  static readonly SECTION_DESC: string = '项目信息'
  static readonly GITHUB_LABEL: string = '项目主页'
  static readonly GITHUB_VALUE: string = 'https://github.com/xiaomu120413/harmony-windows-bridge'
  static readonly FREERDP_LABEL: string = 'FreeRDP 鸿蒙化地址'
  static readonly FREERDP_VALUE: string = 'https://github.com/xiaomu120413/FreeRDP/tree/ohos-port'
  static readonly XRDP_LABEL: string = 'xrdp 鸿蒙化地址'
  static readonly XRDP_VALUE: string = 'https://github.com/xiaomu120413/xrdp/tree/ohos-port'
  static readonly LICENSE_LABEL: string = '许可信息'
  static readonly LICENSE_VALUE: string = 'MIT license'
}
