import {
  SettingsAccent,
  SettingsInfoCard,
  SettingsPageHeader,
  SettingsSectionTitle,
  SettingsTheme
} from './SettingsUi'
import { SettingsText } from './SettingsConstants'

@Component
export struct ProjectHelpPage {
  onBack: () => void = () => {
  }
  @Prop isDark: boolean = false
  @State private aboutCardHovered: boolean = false
  @State private aboutCardPressed: boolean = false

  private setAboutCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.aboutCardHovered = hovered
    })
  }

  private isUrl(value: string): boolean {
    return value.startsWith('http://') || value.startsWith('https://')
  }

  @Builder
  private aboutInfoRow(label: string, value: string) {
    Column() {
      Text(label)
        .fontSize(12)
        .fontColor(SettingsTheme.mutedText(this.isDark))
        .width('100%')
        .margin({ bottom: 4 })
      if (this.isUrl(value)) {
        Hyperlink(value, value)
          .color(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE))
          .width('100%')
      } else {
        Text(value)
          .fontSize(14)
          .fontColor(SettingsTheme.secondaryText(this.isDark))
          .width('100%')
          .lineHeight(20)
      }
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .margin({ bottom: 12 })
  }

  @Builder
  private aboutInfoCard() {
    Column() {
      this.aboutInfoRow(SettingsText.ABOUT_GITHUB_LABEL, SettingsText.ABOUT_GITHUB_VALUE)
      this.aboutInfoRow(SettingsText.ABOUT_FREERDP_ADAPTATION_LABEL,
        SettingsText.ABOUT_FREERDP_ADAPTATION_VALUE)
      this.aboutInfoRow(SettingsText.ABOUT_XRDP_ADAPTATION_LABEL, SettingsText.ABOUT_XRDP_ADAPTATION_VALUE)
      this.aboutInfoRow(SettingsText.ABOUT_LICENSE_LABEL, SettingsText.ABOUT_LICENSE_VALUE)
    }
    .width('100%')
    .padding(16)
    .backgroundColor(this.aboutCardHovered || this.aboutCardPressed ? SettingsTheme.cardHoverBackground(this.isDark) :
      SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.aboutCardHovered))
    .translate({ y: this.aboutCardHovered ? -5 : 0 })
    .scale({
      x: this.aboutCardPressed ? 0.99 : this.aboutCardHovered ? 1.012 : 1,
      y: this.aboutCardPressed ? 0.99 : this.aboutCardHovered ? 1.012 : 1
    })
    .margin({ bottom: 10 })
    .onHover((isHover: boolean) => {
      this.setAboutCardHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.aboutCardPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.aboutCardPressed = false
      }
    })
  }

  build() {
    Column() {
      SettingsPageHeader({
        title: SettingsText.PROJECT_HELP_TITLE,
        subtitle: SettingsText.PROJECT_HELP_SUBTITLE,
        isDark: this.isDark,
        onBack: this.onBack
      })

      Scroll() {
        Column() {
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
            title: SettingsText.USAGE_WINDOWS_RDP_TITLE,
            body: SettingsText.USAGE_WINDOWS_RDP_BODY,
            isDark: this.isDark
          })
          SettingsInfoCard({
            title: SettingsText.USAGE_FORM_TITLE,
            body: SettingsText.USAGE_FORM_BODY,
            isDark: this.isDark
          })
          SettingsInfoCard({
            title: SettingsText.REMOTE_FILES_FEATURE_TITLE,
            body: SettingsText.REMOTE_FILES_FEATURE_DESC,
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

          SettingsSectionTitle({
            title: SettingsText.PROJECT_HELP_ABOUT_SECTION,
            subtitle: SettingsText.ABOUT_SECTION_PROJECT_DESC,
            isDark: this.isDark
          })

          this.aboutInfoCard()
        }
        .alignItems(HorizontalAlign.Start)
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
