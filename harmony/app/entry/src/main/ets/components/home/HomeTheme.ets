import { SettingsAccent, SettingsTheme } from '../settings/SettingsUi'

export class HomeTheme {
  static panelBackground(isDark: boolean): string {
    return SettingsTheme.cardBackground(isDark)
  }

  static appBackground(isDark: boolean): string {
    return isDark ? '#101114' : '#F5F7FA'
  }

  static fieldBackground(isDark: boolean): string {
    return isDark ? '#15171C' : '#FFFFFF'
  }

  static searchBackground(isDark: boolean): string {
    return isDark ? '#10243A' : '#F2F8FF'
  }

  static borderColor(isDark: boolean, active: boolean = false): string {
    return active ? SettingsTheme.accentColor(isDark, SettingsAccent.BLUE) : SettingsTheme.borderColor(isDark)
  }

  static mutedBorderColor(isDark: boolean): string {
    return isDark ? '#303641' : '#D9DEE7'
  }

  static searchBorderColor(isDark: boolean): string {
    return isDark ? '#28567F' : '#C9E4FF'
  }

  static primaryButtonBackground(isDark: boolean, hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return isDark ? '#4E9FFF' : '#086FD8'
    }
    if (hovered) {
      return isDark ? '#65AEFF' : '#087AF0'
    }
    return SettingsTheme.accentColor(isDark, SettingsAccent.BLUE)
  }

  static neutralButtonBackground(isDark: boolean, hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return isDark ? '#2B3038' : '#E6EBF2'
    }
    if (hovered) {
      return isDark ? '#242A33' : '#EEF2F7'
    }
    return isDark ? '#1D2128' : '#F2F4F7'
  }

  static activePanelBackground(isDark: boolean, hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return isDark ? '#162B46' : '#EDF6FF'
    }
    if (hovered) {
      return isDark ? '#1E3B60' : '#EAF4FF'
    }
    return isDark ? '#14263D' : '#F7FBFF'
  }

  static inactivePanelBackground(isDark: boolean, hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return isDark ? '#20242B' : '#EEF2F7'
    }
    if (hovered) {
      return isDark ? '#223043' : '#F2F8FF'
    }
    return HomeTheme.fieldBackground(isDark)
  }
}
