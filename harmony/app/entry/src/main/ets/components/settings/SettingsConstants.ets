import { SettingsAppearanceMode } from './SettingsUi'

export class SettingsRoute {
  static readonly SETTINGS: string = 'settings'
  static readonly APPEARANCE: string = 'appearance'
  static readonly USAGE: string = 'usage'
  static readonly NETWORK: string = 'network'
  static readonly ABOUT: string = 'about'
}

export class SettingsText {
  static readonly HOME_TITLE: string = '鸿蒙与window互联的远程桌面工具'
  static readonly SETTINGS_BUTTON: string = '设置'
  static readonly HOME_HOST_PLACEHOLDER: string = 'Windows host'
  static readonly HOME_PORT_PLACEHOLDER: string = 'Port'
  static readonly HOME_USERNAME_PLACEHOLDER: string = 'Username'
  static readonly HOME_PASSWORD_PLACEHOLDER: string = 'Password'
  static readonly HOME_CERT_TOFU: string = 'TOFU'
  static readonly HOME_CERT_STRICT: string = 'Strict'
  static readonly HOME_CONNECT_BUTTON: string = 'Connect'

  static readonly SETTINGS_TITLE: string = '设置'
  static readonly SETTINGS_SUBTITLE: string = '外观、连接帮助、网络信息和项目信息'
  static readonly SETTINGS_OVERVIEW_TITLE: string = '应用控制台'
  static readonly SETTINGS_OVERVIEW_BODY: string = '集中管理界面外观、连接辅助、本机网络与项目信息。'
  static readonly SETTINGS_OVERVIEW_META: string = '远程桌面客户端设置'
  static readonly SETTINGS_SECTION_GENERAL: string = '常用设置'
  static readonly SETTINGS_SECTION_GENERAL_DESC: string = '调整应用体验和连接前的辅助信息。'
  static readonly SETTINGS_SECTION_PROJECT: string = '项目信息'
  static readonly SETTINGS_SECTION_PROJECT_DESC: string = '查看版本、仓库和许可证状态。'
  static readonly APPEARANCE_ENTRY_TITLE: string = '深色模式'
  static readonly APPEARANCE_ENTRY_DESC: string = '深色模式、浅色模式以及跟随系统'
  static readonly USAGE_ENTRY_TITLE: string = '使用说明'
  static readonly USAGE_ENTRY_DESC: string = '连接准备、参数填写、证书策略和常见排查'
  static readonly NETWORK_ENTRY_TITLE: string = '查看本机 IP'
  static readonly NETWORK_ENTRY_DESC: string = '显示当前默认网络的本机 IP 地址'
  static readonly ABOUT_ENTRY_TITLE: string = '关于'
  static readonly ABOUT_ENTRY_DESC: string = '项目版本、GitHub 地址与开源协议'

  static readonly APPEARANCE_TITLE: string = '深色模式'
  static readonly APPEARANCE_SUBTITLE: string = '选择应用外观。跟随系统会读取当前系统深浅色状态。'
  static readonly APPEARANCE_OVERVIEW_TITLE: string = '当前外观'
  static readonly APPEARANCE_OVERVIEW_BODY: string = '外观设置会同步到主界面和设置页，便于在不同环境下保持可读性。'
  static readonly APPEARANCE_SECTION_MODE: string = '模式选择'
  static readonly APPEARANCE_SECTION_MODE_DESC: string = '选择后立即应用，跟随系统会监听系统深浅色变化。'
  static readonly FOLLOW_SYSTEM_TITLE: string = '跟随系统'
  static readonly FOLLOW_SYSTEM_DESC: string = '读取系统深浅色设置，系统切换后应用外观随之变化。'
  static readonly LIGHT_MODE_TITLE: string = '浅色模式'
  static readonly LIGHT_MODE_DESC: string = '始终使用浅色界面。'
  static readonly DARK_MODE_TITLE: string = '深色模式'
  static readonly DARK_MODE_DESC: string = '始终使用深色界面。'

  static readonly USAGE_TITLE: string = '使用说明'
  static readonly USAGE_SUBTITLE: string = '按当前代码路径整理的连接和排查说明'
  static readonly USAGE_OVERVIEW_TITLE: string = '连接检查清单'
  static readonly USAGE_OVERVIEW_BODY: string = '按连接前、参数填写、证书策略和故障排查组织关键步骤。'
  static readonly USAGE_OVERVIEW_META: string = 'RDP 连接准备'
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
    'TOFU 适合测试和内网首次连接，会在首次连接时信任目标证书；Strict 用于更严格的证书校验。生产环境不要随意忽略证书风险。'
  static readonly USAGE_TROUBLESHOOT_TITLE: string = '常见排查'
  static readonly USAGE_TROUBLESHOOT_BODY: string =
    '连不上时优先确认本机 IP、目标 IP、同一网络、Windows 防火墙、远程桌面开关、账号权限和端口 3389 是否可达。'

  static readonly NETWORK_TITLE: string = '本机 IP'
  static readonly NETWORK_SUBTITLE: string = '显示当前默认网络的本机地址'
  static readonly NETWORK_OVERVIEW_TITLE: string = '默认网络'
  static readonly NETWORK_OVERVIEW_BODY: string = '用于确认当前设备所在网络，排查目标 Windows 主机是否在同一网络段。'
  static readonly NETWORK_OVERVIEW_META: string = '自动读取'
  static readonly NETWORK_SECTION_DETAIL: string = '网络详情'
  static readonly NETWORK_SECTION_DETAIL_DESC: string = '默认网络接口和当前可用 IP 地址。'
  static readonly NETWORK_NAME_LABEL: string = '当前网络'
  static readonly NETWORK_IP_LABEL: string = 'IP 地址'
  static readonly NETWORK_LOADING: string = '查询中'
  static readonly NETWORK_EMPTY: string = '未连接网络'
  static readonly NETWORK_NO_IP: string = '未获取到可用 IP'
  static readonly NETWORK_READ_FAILED: string = '无法读取本机 IP'
  static readonly EMPTY_VALUE: string = '-'

  static readonly ABOUT_TITLE: string = '关于'
  static readonly ABOUT_SUBTITLE: string = '项目、仓库和许可证信息'
  static readonly ABOUT_OVERVIEW_META: string = '项目资料'
  static readonly ABOUT_SECTION_PROJECT: string = '项目详情'
  static readonly ABOUT_SECTION_PROJECT_DESC: string = '正式开源或发布前，需要补齐根目录 LICENSE。'
  static readonly PRODUCT_NAME: string = '木枢'
  static readonly PRODUCT_DESC: string = '基于 FreeRDP 的 HarmonyOS 远程桌面客户端'
  static readonly ABOUT_VERSION_LABEL: string = '版本'
  static readonly ABOUT_BUNDLE_LABEL: string = 'Bundle ID'
  static readonly ABOUT_BUNDLE_VALUE: string = 'com.muhub.desktop'
  static readonly ABOUT_GITHUB_LABEL: string = 'GitHub'
  static readonly ABOUT_GITHUB_VALUE: string = 'https://github.com/xiaomu120413/freerdp-control-demo'
  static readonly ABOUT_LICENSE_LABEL: string = '项目代码许可证'
  static readonly ABOUT_LICENSE_VALUE: string = 'MIT License'

  static appearanceValue(mode: SettingsAppearanceMode): string {
    if (mode === 'dark') {
      return '深色'
    }
    if (mode === 'light') {
      return '浅色'
    }
    return '系统'
  }

}
