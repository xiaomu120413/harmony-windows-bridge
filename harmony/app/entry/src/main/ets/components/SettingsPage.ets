import { AboutPage } from './settings/AboutPage'
import { AppearancePage } from './settings/AppearancePage'
import { NetworkInfoPage } from './settings/NetworkInfoPage'
import { SettingsRoute, SettingsText } from './settings/SettingsConstants'
import {
  SettingsAccent,
  SettingsAppearanceMode,
  SettingsFeatureCard,
  SettingsListItem,
  SettingsPageHeader,
  SettingsResources,
  SettingsSectionTitle,
  SettingsTheme
} from './settings/SettingsUi'
import { UsageGuidePage } from './settings/UsageGuidePage'

@Component
export struct SettingsPage {
  onClose: () => void = () => {
  }
  @State private pageName: string = SettingsRoute.SETTINGS
  @State private appearanceMode: SettingsAppearanceMode = SettingsTheme.getStoredAppearanceMode()
  @StorageLink('settingsSystemDark') private systemDark: boolean = false

  aboutToAppear(): void {
    this.appearanceMode = SettingsTheme.getStoredAppearanceMode()
  }

  private isDarkMode(): boolean {
    if (this.appearanceMode === 'system') {
      return this.systemDark
    }
    return SettingsTheme.isDark(this.getUIContext(), this.appearanceMode)
  }

  @Builder
  private buildSettingsPage() {
    Column() {
      SettingsPageHeader({
        title: SettingsText.SETTINGS_TITLE,
        subtitle: SettingsText.SETTINGS_SUBTITLE,
        isDark: this.isDarkMode(),
        onBack: () => {
          this.onClose()
        }
      })

      Scroll() {
        Column() {
          SettingsFeatureCard({
            title: SettingsText.SETTINGS_OVERVIEW_TITLE,
            body: SettingsText.SETTINGS_OVERVIEW_BODY,
            meta: SettingsText.SETTINGS_OVERVIEW_META,
            iconResource: SettingsResources.DASHBOARD_ICON,
            accentName: SettingsAccent.BLUE,
            isDark: this.isDarkMode()
          })

          SettingsSectionTitle({
            title: SettingsText.SETTINGS_SECTION_GENERAL,
            subtitle: SettingsText.SETTINGS_SECTION_GENERAL_DESC,
            isDark: this.isDarkMode()
          })

          SettingsListItem({
            title: SettingsText.APPEARANCE_ENTRY_TITLE,
            description: SettingsText.APPEARANCE_ENTRY_DESC,
            iconResource: this.appearanceMode === 'dark' ? SettingsResources.MOON_ICON :
              this.appearanceMode === 'light' ? SettingsResources.SUN_ICON : SettingsResources.SYSTEM_ICON,
            accentName: this.appearanceMode === 'dark' ? SettingsAccent.INDIGO :
              this.appearanceMode === 'light' ? SettingsAccent.ORANGE : SettingsAccent.CYAN,
            value: SettingsText.appearanceValue(this.appearanceMode),
            isDark: this.isDarkMode(),
            onPress: () => {
              this.pageName = SettingsRoute.APPEARANCE
            }
          })
          SettingsListItem({
            title: SettingsText.USAGE_ENTRY_TITLE,
            description: SettingsText.USAGE_ENTRY_DESC,
            iconResource: SettingsResources.HELP_ICON,
            accentName: SettingsAccent.PURPLE,
            isDark: this.isDarkMode(),
            onPress: () => {
              this.pageName = SettingsRoute.USAGE
            }
          })
          SettingsListItem({
            title: SettingsText.NETWORK_ENTRY_TITLE,
            description: SettingsText.NETWORK_ENTRY_DESC,
            iconResource: SettingsResources.NETWORK_ICON,
            accentName: SettingsAccent.GREEN,
            isDark: this.isDarkMode(),
            onPress: () => {
              this.pageName = SettingsRoute.NETWORK
            }
          })

          SettingsSectionTitle({
            title: SettingsText.SETTINGS_SECTION_PROJECT,
            subtitle: SettingsText.SETTINGS_SECTION_PROJECT_DESC,
            isDark: this.isDarkMode()
          })

          SettingsListItem({
            title: SettingsText.ABOUT_ENTRY_TITLE,
            description: SettingsText.ABOUT_ENTRY_DESC,
            iconResource: SettingsResources.INFO_ICON,
            accentName: SettingsAccent.AMBER,
            isDark: this.isDarkMode(),
            onPress: () => {
              this.pageName = SettingsRoute.ABOUT
            }
          })
        }
        .padding({ left: 20, right: 20, bottom: 20 })
        .width('100%')
      }
      .layoutWeight(1)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(SettingsTheme.pageBackground(this.isDarkMode()))
  }

  build() {
    Column() {
      if (this.pageName === SettingsRoute.APPEARANCE) {
        AppearancePage({
          onModeChange: (mode: SettingsAppearanceMode) => {
            this.appearanceMode = mode
          },
          onBack: () => {
            this.pageName = SettingsRoute.SETTINGS
          }
        })
      } else if (this.pageName === SettingsRoute.USAGE) {
        UsageGuidePage({
          isDark: this.isDarkMode(),
          onBack: () => {
            this.pageName = SettingsRoute.SETTINGS
          }
        })
      } else if (this.pageName === SettingsRoute.NETWORK) {
        NetworkInfoPage({
          isDark: this.isDarkMode(),
          onBack: () => {
            this.pageName = SettingsRoute.SETTINGS
          }
        })
      } else if (this.pageName === SettingsRoute.ABOUT) {
        AboutPage({
          isDark: this.isDarkMode(),
          onBack: () => {
            this.pageName = SettingsRoute.SETTINGS
          }
        })
      } else {
        this.buildSettingsPage()
      }
    }
    .width('100%')
    .height('100%')
  }
}
