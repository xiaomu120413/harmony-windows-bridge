import { common, ConfigurationConstant } from '@kit.AbilityKit'

export type SettingsAppearanceMode = 'system' | 'light' | 'dark'
export type SettingsAccentName = 'blue' | 'purple' | 'green' | 'amber' | 'cyan' | 'orange' | 'indigo'
export type SettingsStatusTone = 'ok' | 'warning' | 'danger' | 'info' | 'neutral'
export const SETTINGS_STORAGE_SYSTEM_DARK: string = 'settingsSystemDark'
export const SETTINGS_STORAGE_APPEARANCE_MODE: string = 'settingsAppearanceMode'

export class SettingsResources {
  static readonly ARROW_LEFT_ICON: Resource = $r('app.media.arrow_left')
  static readonly APP_ICON: Resource = $r('app.media.icon')
  static readonly CHECK_ICON: Resource = $r('app.media.check')
  static readonly DASHBOARD_ICON: Resource = $r('app.media.dashboard')
  static readonly SETTINGS_ICON: Resource = $r('app.media.settings')
  static readonly HELP_ICON: Resource = $r('app.media.help_menu')
  static readonly NETWORK_ICON: Resource = $r('app.media.network')
  static readonly INFO_ICON: Resource = $r('app.media.info_menu')
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

  static inputBackground(isDark: boolean): string {
    return isDark ? '#15171C' : '#F8FAFC'
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

@Component
export struct SettingsStatusChip {
  @Prop text: string = ''
  @Prop tone: SettingsStatusTone = 'neutral'
  @Prop isDark: boolean = false

  build() {
    Row({ space: 6 }) {
      Circle()
        .width(7)
        .height(7)
        .fill(SettingsTheme.statusText(this.isDark, this.tone))

      Text(this.text)
        .fontSize(11)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.statusText(this.isDark, this.tone))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
    }
    .height(28)
    .padding({ left: 10, right: 10 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(SettingsTheme.statusBackground(this.isDark, this.tone))
    .borderRadius(14)
    .border({
      width: 1,
      color: SettingsTheme.statusBorder(this.isDark, this.tone)
    })
  }
}

@Component
export struct SettingsDesktopNavItem {
  @Prop title: string = ''
  @Prop description: string = ''
  @Prop value: string = ''
  @Prop iconResource: Resource = SettingsResources.SETTINGS_ICON
  @Prop accentName: SettingsAccentName = SettingsAccent.BLUE
  @Prop isDark: boolean = false
  @Prop selected: boolean = false
  onPress: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    this.hovered = hovered
  }

  build() {
    Row({ space: 10 }) {
      Column() {
        Image(this.iconResource)
          .width(17)
          .height(17)
          .fillColor(this.selected ? Color.White : SettingsTheme.accentColor(this.isDark, this.accentName))
          .draggable(false)
      }
      .width(32)
      .height(32)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(10)
      .backgroundColor(this.selected ? SettingsTheme.accentColor(this.isDark, this.accentName) :
        SettingsTheme.accentBackground(this.isDark, this.accentName))

      Text(this.title)
        .fontSize(14)
        .fontWeight(this.selected ? FontWeight.Bold : FontWeight.Medium)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .layoutWeight(1)
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .height(46)
    .padding({ left: 10, right: 10 })
    .backgroundColor(SettingsTheme.transparentButton(this.selected || this.hovered, this.pressed, this.isDark))
    .borderRadius(12)
    .onHover((isHover: boolean) => {
      this.setHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressed = false
      }
    })
    .onClick(() => {
      this.pressed = false
      this.onPress()
    })
  }
}

@Component
export struct SettingsKeyValueRow {
  @Prop label: string = ''
  @Prop value: string = ''
  @Prop isDark: boolean = false
  @Prop tone: SettingsStatusTone = 'neutral'

  build() {
    Row() {
      Text(this.label)
        .fontSize(12)
        .fontColor(SettingsTheme.mutedText(this.isDark))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .layoutWeight(1)

      Text(this.value)
        .fontSize(13)
        .fontWeight(FontWeight.Medium)
        .fontColor(this.tone === 'neutral' ? SettingsTheme.primaryText(this.isDark) :
          SettingsTheme.statusText(this.isDark, this.tone))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .textAlign(TextAlign.End)
        .margin({ left: 12 })
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .height(32)
  }
}

@Component
export struct SettingsCompactButton {
  @Prop text: string = ''
  @Prop isDark: boolean = false
  @Prop busy: boolean = false
  @Prop primary: boolean = false
  onPress: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  build() {
    Button(this.text)
      .type(ButtonType.Normal)
      .height(34)
      .borderRadius(10)
      .padding({ left: 14, right: 14 })
      .backgroundColor(this.primary ? '#0A84FF' : SettingsTheme.subtleButton(this.isDark, this.hovered,
        this.pressed || this.busy))
      .fontColor(this.primary ? Color.White : SettingsTheme.primaryText(this.isDark))
      .fontSize(12)
      .stateEffect(false)
      .opacity(this.busy ? 0.7 : 1)
      .onHover((isHover: boolean) => {
        this.hovered = isHover
      })
      .onTouch((event: TouchEvent) => {
        if (event.type === TouchType.Down) {
          this.pressed = true
          return
        }
        if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
          this.pressed = false
        }
      })
      .onClick(() => {
        if (this.busy) {
          return
        }
        this.pressed = false
        this.onPress()
      })
  }
}

@Component
export struct SettingsBackButton {
  @Prop isDark: boolean = false
  onBack: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    this.hovered = hovered
  }

  build() {
    Button() {
      Image(SettingsResources.ARROW_LEFT_ICON)
        .width(18)
        .height(18)
        .fillColor(SettingsTheme.primaryText(this.isDark))
        .draggable(false)
    }
    .type(ButtonType.Normal)
    .width(44)
    .height(44)
    .borderRadius(SettingsTheme.BUTTON_RADIUS)
    .backgroundColor(SettingsTheme.transparentButton(this.hovered, this.pressed, this.isDark))
    .stateEffect(false)
    .onHover((isHover: boolean) => {
      this.setHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressed = false
      }
    })
    .onClick(() => {
      this.pressed = false
      this.onBack()
    })
  }
}

@Component
export struct SettingsPageHeader {
  @Prop title: string = ''
  @Prop subtitle: string = ''
  @Prop isDark: boolean = false
  onBack: () => void = () => {
  }

  build() {
    Row() {
      Row({ space: 8 }) {
        SettingsBackButton({
          isDark: this.isDark,
          onBack: this.onBack
        })

        Column({ space: 4 }) {
          Text(this.title)
            .fontSize(24)
            .fontWeight(FontWeight.Bold)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')

          if (this.subtitle.length > 0) {
            Text(this.subtitle)
              .fontSize(12)
              .fontColor(SettingsTheme.mutedText(this.isDark))
              .maxLines(2)
              .textOverflow({ overflow: TextOverflow.Ellipsis })
              .width('100%')
          }
        }
        .alignItems(HorizontalAlign.Start)
        .layoutWeight(1)
      }
      .layoutWeight(1)
      .alignItems(VerticalAlign.Top)
    }
    .width('100%')
    .alignItems(VerticalAlign.Top)
    .padding({ left: 16, right: 20, top: 16, bottom: 14 })
  }
}

@Component
export struct SettingsListItem {
  @Prop title: string = ''
  @Prop description: string = ''
  @Prop iconResource: Resource = SettingsResources.SETTINGS_ICON
  @Prop accentName: SettingsAccentName = SettingsAccent.BLUE
  @Prop value: string = ''
  @Prop isDark: boolean = false
  @Prop selected: boolean = false
  onPress: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  build() {
    Row({ space: 14 }) {
      Column() {
        Image(this.iconResource)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(this.selected ? Color.White : SettingsTheme.accentColor(this.isDark, this.accentName))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(this.selected ? SettingsTheme.accentColor(this.isDark, this.accentName) :
        (this.hovered || this.pressed ? SettingsTheme.accentHoverBackground(this.isDark, this.accentName) :
          SettingsTheme.accentBackground(this.isDark, this.accentName)))

      Column({ space: 5 }) {
        Text(this.title)
          .fontSize(15)
          .fontWeight(this.selected ? FontWeight.Bold : FontWeight.Medium)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
        Text(this.description)
          .fontSize(13)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .maxLines(2)
          .lineHeight(19)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)

      if (this.selected) {
        Image(SettingsResources.CHECK_ICON)
          .width(18)
          .height(18)
          .fillColor(SettingsTheme.accentColor(this.isDark, this.accentName))
          .margin({ left: 6 })
          .draggable(false)
      } else if (this.value.length > 0) {
        Text(this.value)
          .fontSize(12)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .margin({ left: 6 })
      }
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding({ left: 14, right: 14, top: 12, bottom: 12 })
    .backgroundColor(this.hovered || this.pressed ? SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: this.selected ? SettingsTheme.accentColor(this.isDark, this.accentName) :
        SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered))
    .translate({ y: this.hovered ? -5 : 0 })
    .scale({
      x: this.pressed ? 0.99 : this.hovered ? 1.012 : 1,
      y: this.pressed ? 0.99 : this.hovered ? 1.012 : 1
    })
    .margin({ bottom: 10 })
    .onHover((isHover: boolean) => {
      this.setHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressed = false
      }
    })
    .onClick(() => {
      this.pressed = false
      this.onPress()
    })
  }
}

@Component
export struct SettingsSectionTitle {
  @Prop title: string = ''
  @Prop subtitle: string = ''
  @Prop isDark: boolean = false

  build() {
    Column({ space: 4 }) {
      Text(this.title)
        .fontSize(13)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width('100%')
      if (this.subtitle.length > 0) {
        Text(this.subtitle)
          .fontSize(12)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .width('100%')
          .lineHeight(18)
      }
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .margin({ top: 6, bottom: 10 })
  }
}

@Component
export struct SettingsFeatureCard {
  @Prop title: string = ''
  @Prop body: string = ''
  @Prop meta: string = ''
  @Prop iconResource: Resource = SettingsResources.SETTINGS_ICON
  @Prop accentName: SettingsAccentName = SettingsAccent.BLUE
  @Prop isDark: boolean = false
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  build() {
    Row({ space: 14 }) {
      Column() {
        Image(this.iconResource)
          .width(24)
          .height(24)
          .fillColor(SettingsTheme.accentColor(this.isDark, this.accentName))
          .draggable(false)
      }
      .width(46)
      .height(46)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(this.hovered || this.pressed ? SettingsTheme.accentHoverBackground(this.isDark,
        this.accentName) : SettingsTheme.accentBackground(this.isDark, this.accentName))

      Column({ space: 7 }) {
        Text(this.title)
          .fontSize(17)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        Text(this.body)
          .fontSize(13)
          .fontColor(SettingsTheme.secondaryText(this.isDark))
          .lineHeight(20)
          .width('100%')
          .maxLines(3)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        if (this.meta.length > 0) {
          Text(this.meta)
            .fontSize(12)
            .fontColor(SettingsTheme.accentColor(this.isDark, this.accentName))
            .width('100%')
        }
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding(18)
    .backgroundColor(this.hovered || this.pressed ? SettingsTheme.cardHoverBackground(this.isDark) :
      SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered))
    .translate({ y: this.hovered ? -5 : 0 })
    .scale({
      x: this.pressed ? 0.99 : this.hovered ? 1.012 : 1,
      y: this.pressed ? 0.99 : this.hovered ? 1.012 : 1
    })
    .margin({ bottom: 14 })
    .onHover((isHover: boolean) => {
      this.setHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressed = false
      }
    })
  }
}

@Component
export struct SettingsInfoCard {
  @Prop title: string = ''
  @Prop body: string = ''
  @Prop isDark: boolean = false
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  build() {
    Column() {
      Text(this.title)
        .fontSize(15)
        .fontWeight(FontWeight.Medium)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width('100%')
        .margin({ bottom: 8 })
      Text(this.body)
        .fontSize(14)
        .fontColor(SettingsTheme.secondaryText(this.isDark))
        .lineHeight(22)
        .width('100%')
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .padding(16)
    .backgroundColor(this.hovered || this.pressed ? SettingsTheme.cardHoverBackground(this.isDark) :
      SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered))
    .translate({ y: this.hovered ? -5 : 0 })
    .scale({
      x: this.pressed ? 0.99 : this.hovered ? 1.012 : 1,
      y: this.pressed ? 0.99 : this.hovered ? 1.012 : 1
    })
    .margin({ bottom: 10 })
    .onHover((isHover: boolean) => {
      this.setHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressed = false
      }
    })
  }
}
