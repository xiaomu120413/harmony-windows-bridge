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
export struct BasicSettingsPage {
  onBack: () => void = () => {
  }
  onModeChange: (mode: SettingsAppearanceMode) => void = (_mode: SettingsAppearanceMode) => {
  }
  @State private localMode: SettingsAppearanceMode = SettingsTheme.getStoredAppearanceMode()
  @StorageLink('settingsSystemDark') private systemDark: boolean = false

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

  private applyColorMode(mode: SettingsAppearanceMode): void {
    const previousMode = this.localMode
    this.localMode = mode
    if (SettingsTheme.applyAppearanceMode(this.getUIContext(), mode)) {
      this.onModeChange(mode)
      return
    }
    this.localMode = previousMode
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
        title: SettingsText.BASIC_SETTINGS_TITLE,
        subtitle: SettingsText.BASIC_SETTINGS_SUBTITLE,
        isDark: this.isVisualDark(),
        onBack: this.onBack
      })

      Scroll() {
        Column() {
          SettingsSectionTitle({
            title: SettingsText.BASIC_APPEARANCE_SECTION,
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
