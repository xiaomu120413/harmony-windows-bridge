import { SettingsAccentName } from './SettingsTypes'

export class SettingsResources {
  static readonly ARROW_LEFT_ICON: Resource = $r('app.media.arrow_left')
  static readonly CHECK_ICON: Resource = $r('app.media.check')
  static readonly DASHBOARD_ICON: Resource = $r('app.media.dashboard')
  static readonly SETTINGS_ICON: Resource = $r('app.media.settings')
  static readonly HELP_ICON: Resource = $r('app.media.help_menu')
  static readonly SYSTEM_ICON: Resource = $r('app.media.globe')
  static readonly SUN_ICON: Resource = $r('app.media.sun')
  static readonly MOON_ICON: Resource = $r('app.media.moon')
}

export class SettingsAccent {
  static readonly BLUE: SettingsAccentName = 'blue'
  static readonly PURPLE: SettingsAccentName = 'purple'
  static readonly GREEN: SettingsAccentName = 'green'
  static readonly AMBER: SettingsAccentName = 'amber'
  static readonly CYAN: SettingsAccentName = 'cyan'
  static readonly ORANGE: SettingsAccentName = 'orange'
  static readonly INDIGO: SettingsAccentName = 'indigo'
}
