import {
  SettingsAccent,
  SettingsFeatureCard,
  SettingsInfoCard,
  SettingsPageHeader,
  SettingsResources,
  SettingsSectionTitle,
  SettingsTheme
} from './SettingsUi'
import { SettingsText } from './SettingsConstants'

@Component
export struct UsageGuidePage {
  onBack: () => void = () => {
  }
  @Prop isDark: boolean = false

  build() {
    Column() {
      SettingsPageHeader({
        title: SettingsText.USAGE_TITLE,
        subtitle: SettingsText.USAGE_SUBTITLE,
        isDark: this.isDark,
        onBack: this.onBack
      })

      Scroll() {
        Column() {
          SettingsFeatureCard({
            title: SettingsText.USAGE_OVERVIEW_TITLE,
            body: SettingsText.USAGE_OVERVIEW_BODY,
            meta: SettingsText.USAGE_OVERVIEW_META,
            iconResource: SettingsResources.HELP_ICON,
            accentName: SettingsAccent.PURPLE,
            isDark: this.isDark
          })

          SettingsSectionTitle({
            title: SettingsText.USAGE_SECTION_PREP,
            subtitle: SettingsText.USAGE_SECTION_PREP_DESC,
            isDark: this.isDark
          })

          SettingsInfoCard({
            title: SettingsText.USAGE_PREP_TITLE,
            body: SettingsText.USAGE_PREP_BODY,
            isDark: this.isDark
          })
          SettingsInfoCard({
            title: SettingsText.USAGE_FORM_TITLE,
            body: SettingsText.USAGE_FORM_BODY,
            isDark: this.isDark
          })
          SettingsInfoCard({
            title: SettingsText.USAGE_REMOTE_FILES_TITLE,
            body: SettingsText.USAGE_REMOTE_FILES_BODY,
            isDark: this.isDark
          })

          SettingsSectionTitle({
            title: SettingsText.USAGE_SECTION_SECURITY,
            subtitle: SettingsText.USAGE_SECTION_SECURITY_DESC,
            isDark: this.isDark
          })

          SettingsInfoCard({
            title: SettingsText.USAGE_CERT_TITLE,
            body: SettingsText.USAGE_CERT_BODY,
            isDark: this.isDark
          })
          SettingsInfoCard({
            title: SettingsText.USAGE_TROUBLESHOOT_TITLE,
            body: SettingsText.USAGE_TROUBLESHOOT_BODY,
            isDark: this.isDark
          })
        }
        .padding({ left: 20, right: 20, bottom: 24 })
        .width('100%')
      }
      .layoutWeight(1)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(SettingsTheme.pageBackground(this.isDark))
  }
}
