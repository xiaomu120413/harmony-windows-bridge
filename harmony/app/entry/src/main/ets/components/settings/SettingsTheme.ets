import { common, ConfigurationConstant } from '@kit.AbilityKit'
import { SettingsAccent } from './SettingsResources'
import {
  SETTINGS_STORAGE_APPEARANCE_MODE,
  SETTINGS_STORAGE_SYSTEM_DARK,
  SettingsAccentName,
  SettingsAppearanceMode,
  SettingsStatusTone
} from './SettingsTypes'

export class SettingsTheme {
  static readonly MOTION_MS: number = 180
  static readonly CARD_RADIUS: number = 12
  static readonly BUTTON_RADIUS: number = 12
  static readonly ICON_TILE_SIZE: number = 38
  static readonly ICON_SIZE: number = 20

  static normalizeAppearanceMode(mode: string | undefined): SettingsAppearanceMode {
    if (mode === 'dark') {
      return 'dark'
    }
    if (mode === 'light') {
      return 'light'
    }
    return 'system'
  }

  static getStoredAppearanceMode(): SettingsAppearanceMode {
    return SettingsTheme.normalizeAppearanceMode(AppStorage.get<string>(SETTINGS_STORAGE_APPEARANCE_MODE))
  }

  static detectSystemDark(context: common.UIAbilityContext): boolean | null {
    const colorMode = context.config?.colorMode
    if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK) {
      return true
    }
    if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT) {
      return false
    }
    return null
  }

  static initAppearance(context: common.UIAbilityContext): void {
    const detected = SettingsTheme.detectSystemDark(context)
    const systemDark = detected === null ? false : detected
    AppStorage.setOrCreate<boolean>(SETTINGS_STORAGE_SYSTEM_DARK, systemDark)
    AppStorage.setOrCreate<SettingsAppearanceMode>(SETTINGS_STORAGE_APPEARANCE_MODE,
      SettingsTheme.getStoredAppearanceMode())
  }

  static updateSystemAppearance(colorMode: ConfigurationConstant.ColorMode | undefined): void {
    if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK) {
      AppStorage.setOrCreate<boolean>(SETTINGS_STORAGE_SYSTEM_DARK, true)
      return
    }
    if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT) {
      AppStorage.setOrCreate<boolean>(SETTINGS_STORAGE_SYSTEM_DARK, false)
    }
  }

  static colorModeForAppearance(mode: SettingsAppearanceMode): ConfigurationConstant.ColorMode {
    if (mode === 'dark') {
      return ConfigurationConstant.ColorMode.COLOR_MODE_DARK
    }
    if (mode === 'light') {
      return ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT
    }
    return ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET
  }

  static applyAppearanceModeToApplication(context: common.UIAbilityContext, mode: SettingsAppearanceMode): boolean {
    try {
      context.getApplicationContext().setColorMode(SettingsTheme.colorModeForAppearance(mode))
      return true
    } catch (_error) {
      return false
    }
  }

  static applyAppearanceMode(uiContext: UIContext, mode: SettingsAppearanceMode): boolean {
    const hostContext = uiContext.getHostContext()
    if (!hostContext) {
      return false
    }

    const abilityContext = hostContext as common.UIAbilityContext
    AppStorage.setOrCreate<SettingsAppearanceMode>(SETTINGS_STORAGE_APPEARANCE_MODE, mode)
    return SettingsTheme.applyAppearanceModeToApplication(abilityContext, mode)
  }

  static isSystemDark(uiContext: UIContext): boolean {
    const hostContext = uiContext.getHostContext()
    if (!hostContext) {
      return AppStorage.get<boolean>(SETTINGS_STORAGE_SYSTEM_DARK) === true
    }

    const abilityContext = hostContext as common.UIAbilityContext
    const detected = SettingsTheme.detectSystemDark(abilityContext)
    if (detected !== null) {
      AppStorage.setOrCreate<boolean>(SETTINGS_STORAGE_SYSTEM_DARK, detected)
      return detected
    }
    return AppStorage.get<boolean>(SETTINGS_STORAGE_SYSTEM_DARK) === true
  }

  static isDark(uiContext: UIContext, mode: SettingsAppearanceMode): boolean {
    if (mode === 'dark') {
      return true
    }
    if (mode === 'light') {
      return false
    }
    return SettingsTheme.isSystemDark(uiContext)
  }

  static animate(uiContext: UIContext, update: () => void): void {
    uiContext.animateTo({
      duration: SettingsTheme.MOTION_MS,
      curve: Curve.EaseOut
    }, update)
  }

  static pageBackground(isDark: boolean): string {
    return isDark ? '#101114' : '#F6F7F9'
  }

  static cardBackground(isDark: boolean): string {
    return isDark ? '#181A1F' : '#FFFFFF'
  }

  static cardHoverBackground(isDark: boolean): string {
    return isDark ? '#20232A' : '#F8FAFC'
  }

  static primaryText(isDark: boolean): string {
    return isDark ? '#F8FAFC' : '#101828'
  }

  static secondaryText(isDark: boolean): string {
    return isDark ? '#D1D5DB' : '#374151'
  }

  static mutedText(isDark: boolean): string {
    return isDark ? '#9CA3AF' : '#6B7280'
  }

  static disabledText(isDark: boolean): string {
    return isDark ? '#6B7280' : '#A0A7B2'
  }

  static accentColor(isDark: boolean, accentName: SettingsAccentName): string {
    if (accentName === SettingsAccent.PURPLE) {
      return isDark ? '#C4B5FD' : '#7C3AED'
    }
    if (accentName === SettingsAccent.GREEN) {
      return isDark ? '#86EFAC' : '#16A34A'
    }
    if (accentName === SettingsAccent.AMBER) {
      return isDark ? '#FCD34D' : '#D97706'
    }
    if (accentName === SettingsAccent.CYAN) {
      return isDark ? '#67E8F9' : '#0891B2'
    }
    if (accentName === SettingsAccent.ORANGE) {
      return isDark ? '#FDBA74' : '#EA580C'
    }
    if (accentName === SettingsAccent.INDIGO) {
      return isDark ? '#A5B4FC' : '#4F46E5'
    }
    return isDark ? '#72B7FF' : '#0A84FF'
  }

  static accentBackground(isDark: boolean, accentName: SettingsAccentName): string {
    if (accentName === SettingsAccent.PURPLE) {
      return isDark ? '#35245F' : '#F3E8FF'
    }
    if (accentName === SettingsAccent.GREEN) {
      return isDark ? '#173F2A' : '#EAF8EF'
    }
    if (accentName === SettingsAccent.AMBER) {
      return isDark ? '#4A3718' : '#FFF4D6'
    }
    if (accentName === SettingsAccent.CYAN) {
      return isDark ? '#153F47' : '#E8FAFC'
    }
    if (accentName === SettingsAccent.ORANGE) {
      return isDark ? '#4A2C16' : '#FFF0E5'
    }
    if (accentName === SettingsAccent.INDIGO) {
      return isDark ? '#272D61' : '#EEF2FF'
    }
    return isDark ? '#173A5E' : '#EAF3FF'
  }

  static accentHoverBackground(isDark: boolean, accentName: SettingsAccentName): string {
    if (accentName === SettingsAccent.PURPLE) {
      return isDark ? '#402A73' : '#EDE2FF'
    }
    if (accentName === SettingsAccent.GREEN) {
      return isDark ? '#1C4B32' : '#DDF5E7'
    }
    if (accentName === SettingsAccent.AMBER) {
      return isDark ? '#5A421E' : '#FFEBC2'
    }
    if (accentName === SettingsAccent.CYAN) {
      return isDark ? '#1A4A53' : '#DCF7FA'
    }
    if (accentName === SettingsAccent.ORANGE) {
      return isDark ? '#5A341A' : '#FFE5D4'
    }
    if (accentName === SettingsAccent.INDIGO) {
      return isDark ? '#303574' : '#E4E9FF'
    }
    return isDark ? '#1C456E' : '#DDEEFF'
  }

  static borderColor(isDark: boolean, active: boolean = false): string {
    if (active) {
      return '#0A84FF'
    }
    return isDark ? '#2B3038' : '#E5E7EB'
  }

  static transparentButton(hovered: boolean, pressed: boolean, isDark: boolean): string {
    if (!hovered && !pressed) {
      return 'rgba(0,0,0,0)'
    }
    if (pressed) {
      return isDark ? 'rgba(255,255,255,0.14)' : 'rgba(0,0,0,0.08)'
    }
    return isDark ? 'rgba(255,255,255,0.10)' : 'rgba(0,0,0,0.05)'
  }

  static subtleButton(isDark: boolean, hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return isDark ? '#2A3038' : '#E5EAF0'
    }
    if (hovered) {
      return isDark ? '#242932' : '#EEF2F7'
    }
    return isDark ? '#1D2128' : '#F2F4F7'
  }

  static disabledButton(isDark: boolean): string {
    return isDark ? '#1A1D23' : '#F6F7F9'
  }

  static shadow(isDark: boolean, hovered: boolean): ShadowOptions {
    return {
      radius: hovered ? 16 : 6,
      color: isDark ? (hovered ? '#44000000' : '#1A000000') : (hovered ? '#12111827' : '#06111827'),
      offsetX: 0,
      offsetY: hovered ? 8 : 2
    }
  }

  static statusText(isDark: boolean, tone: SettingsStatusTone): string {
    if (tone === 'ok') {
      return isDark ? '#86EFAC' : '#15803D'
    }
    if (tone === 'warning') {
      return isDark ? '#FCD34D' : '#B45309'
    }
    if (tone === 'danger') {
      return isDark ? '#FCA5A5' : '#B91C1C'
    }
    if (tone === 'info') {
      return isDark ? '#93C5FD' : '#0A84FF'
    }
    return isDark ? '#D1D5DB' : '#475467'
  }

  static statusBackground(isDark: boolean, tone: SettingsStatusTone): string {
    if (tone === 'ok') {
      return isDark ? '#173F2A' : '#EAF8EF'
    }
    if (tone === 'warning') {
      return isDark ? '#4A3718' : '#FFF4D6'
    }
    if (tone === 'danger') {
      return isDark ? '#4A1F1F' : '#FEECEC'
    }
    if (tone === 'info') {
      return isDark ? '#173A5E' : '#EAF3FF'
    }
    return isDark ? '#242932' : '#F2F4F7'
  }

  static statusBorder(isDark: boolean, tone: SettingsStatusTone): string {
    if (tone === 'ok') {
      return isDark ? '#24563A' : '#C8EBD5'
    }
    if (tone === 'warning') {
      return isDark ? '#6B531E' : '#F6D98B'
    }
    if (tone === 'danger') {
      return isDark ? '#733232' : '#F5B8B8'
    }
    if (tone === 'info') {
      return isDark ? '#24517C' : '#B8DBFF'
    }
    return SettingsTheme.borderColor(isDark)
  }
}
