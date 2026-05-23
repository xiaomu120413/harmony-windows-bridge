import {
  SettingsAccent,
  SettingsAccentName,
  SettingsAppearanceMode,
  SettingsListItem,
  SettingsPageHeader,
  SettingsResources,
  SettingsSectionTitle,
  SettingsTheme
} from './SettingsUi'
import { SettingsText } from './SettingsConstants'

@Component
export struct AppearancePage {
  onBack: () => void = () => {
  }
  onModeChange: (mode: SettingsAppearanceMode) => void = (_mode: SettingsAppearanceMode) => {
  }
  @State private localMode: SettingsAppearanceMode = SettingsTheme.getStoredAppearanceMode()
  @StorageLink('settingsSystemDark') private systemDark: boolean = false
  @State private overviewHovered: boolean = false
  @State private overviewPressed: boolean = false

  aboutToAppear(): void {
    this.localMode = SettingsTheme.getStoredAppearanceMode()
  }

  private resolveVisualDark(mode: SettingsAppearanceMode): boolean {
    if (mode === 'dark') {
      return true
    }
    if (mode === 'light') {
      return false
    }
    return this.systemDark
  }

  private isVisualDark(): boolean {
    return this.resolveVisualDark(this.localMode)
  }

  private currentAccent(): SettingsAccentName {
    if (this.localMode === 'dark') {
      return SettingsAccent.INDIGO
    }
    if (this.localMode === 'light') {
      return SettingsAccent.ORANGE
    }
    return SettingsAccent.CYAN
  }

  private currentIcon(): Resource {
    if (this.localMode === 'dark') {
      return SettingsResources.MOON_ICON
    }
    if (this.localMode === 'light') {
      return SettingsResources.SUN_ICON
    }
    return SettingsResources.SYSTEM_ICON
  }

  private applyColorMode(mode: SettingsAppearanceMode): void {
    const previousMode = this.localMode
    this.localMode = mode
    this.overviewPressed = false
    if (SettingsTheme.applyAppearanceMode(this.getUIContext(), mode)) {
      this.onModeChange(mode)
      return
    }
    this.localMode = previousMode
  }

  private setOverviewHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.overviewHovered = hovered
    })
  }

  @Builder
  private overviewCard() {
    Row({ space: 14 }) {
      Column() {
        Image(this.currentIcon())
          .width(24)
          .height(24)
          .fillColor(SettingsTheme.accentColor(this.isVisualDark(), this.currentAccent()))
          .draggable(false)
      }
      .width(46)
      .height(46)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(this.overviewHovered || this.overviewPressed ?
        SettingsTheme.accentHoverBackground(this.isVisualDark(), this.currentAccent()) :
        SettingsTheme.accentBackground(this.isVisualDark(), this.currentAccent()))

      Column({ space: 7 }) {
        Text(SettingsText.APPEARANCE_OVERVIEW_TITLE)
          .fontSize(17)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isVisualDark()))
          .width('100%')
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        Text(SettingsText.APPEARANCE_OVERVIEW_BODY)
          .fontSize(13)
          .fontColor(SettingsTheme.secondaryText(this.isVisualDark()))
          .lineHeight(20)
          .width('100%')
          .maxLines(3)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding(18)
    .backgroundColor(this.overviewHovered || this.overviewPressed ? SettingsTheme.cardHoverBackground(this.isVisualDark()) :
      SettingsTheme.cardBackground(this.isVisualDark()))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isVisualDark())
    })
    .shadow(SettingsTheme.shadow(this.isVisualDark(), this.overviewHovered))
    .translate({ y: this.overviewHovered ? -5 : 0 })
    .scale({
      x: this.overviewPressed ? 0.99 : this.overviewHovered ? 1.012 : 1,
      y: this.overviewPressed ? 0.99 : this.overviewHovered ? 1.012 : 1
    })
    .margin({ bottom: 14 })
    .onHover((isHover: boolean) => {
      this.setOverviewHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.overviewPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.overviewPressed = false
      }
    })
  }

  @Builder
  private modeOption(mode: SettingsAppearanceMode, title: string, description: string, iconResource: Resource,
    accentName: SettingsAccentName) {
    SettingsListItem({
      title: title,
      description: description,
      iconResource: iconResource,
      accentName: accentName,
      selected: this.localMode === mode,
      isDark: this.isVisualDark(),
      onPress: () => {
        this.applyColorMode(mode)
      }
    })
  }

  build() {
    Column() {
      SettingsPageHeader({
        title: SettingsText.APPEARANCE_TITLE,
        subtitle: SettingsText.APPEARANCE_SUBTITLE,
        isDark: this.isVisualDark(),
        onBack: this.onBack
      })

      Scroll() {
        Column() {
          this.overviewCard()

          SettingsSectionTitle({
            title: SettingsText.APPEARANCE_SECTION_MODE,
            subtitle: SettingsText.APPEARANCE_SECTION_MODE_DESC,
            isDark: this.isVisualDark()
          })

          this.modeOption('system', SettingsText.FOLLOW_SYSTEM_TITLE, SettingsText.FOLLOW_SYSTEM_DESC,
            SettingsResources.SYSTEM_ICON, SettingsAccent.CYAN)
          this.modeOption('light', SettingsText.LIGHT_MODE_TITLE, SettingsText.LIGHT_MODE_DESC,
            SettingsResources.SUN_ICON, SettingsAccent.ORANGE)
          this.modeOption('dark', SettingsText.DARK_MODE_TITLE, SettingsText.DARK_MODE_DESC,
            SettingsResources.MOON_ICON, SettingsAccent.INDIGO)
        }
        .alignItems(HorizontalAlign.Start)
        .padding({ left: 20, right: 20, bottom: 24 })
        .width('100%')
      }
      .layoutWeight(1)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(SettingsTheme.pageBackground(this.isVisualDark()))
  }
}
