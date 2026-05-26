import { bundleManager } from '@kit.AbilityKit'
import {
  SettingsAccent,
  SettingsFeatureCard,
  SettingsPageHeader,
  SettingsResources,
  SettingsSectionTitle,
  SettingsTheme
} from './SettingsUi'
import { SettingsText } from './SettingsConstants'

@Component
export struct AboutPage {
  onBack: () => void = () => {
  }
  @Prop isDark: boolean = false
  @State private cardHovered: boolean = false
  @State private cardPressed: boolean = false
  @State private versionValue: string = SettingsText.EMPTY_VALUE

  aboutToAppear(): void {
    this.loadBundleInfo()
  }

  private loadBundleInfo(): void {
    try {
      const bundleInfo = bundleManager.getBundleInfoForSelfSync(
        bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION
      )
      this.versionValue = `${bundleInfo.versionName} (${bundleInfo.versionCode})`
    } catch (_error) {
      this.versionValue = SettingsText.EMPTY_VALUE
    }
  }

  private setCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.cardHovered = hovered
    })
  }

  @Builder
  private aboutInfoRow(label: string, value: string) {
    Column() {
      Text(label)
        .fontSize(12)
        .fontColor(SettingsTheme.mutedText(this.isDark))
        .width('100%')
        .margin({ bottom: 4 })
      Text(value)
        .fontSize(14)
        .fontColor(SettingsTheme.secondaryText(this.isDark))
        .width('100%')
        .lineHeight(20)
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .margin({ bottom: 12 })
  }

  build() {
    Column() {
      SettingsPageHeader({
        title: SettingsText.ABOUT_TITLE,
        subtitle: SettingsText.ABOUT_SUBTITLE,
        isDark: this.isDark,
        onBack: this.onBack
      })

      Scroll() {
        Column() {
          SettingsFeatureCard({
            title: SettingsText.PRODUCT_NAME,
            body: SettingsText.PRODUCT_DESC,
            meta: SettingsText.ABOUT_OVERVIEW_META,
            iconResource: SettingsResources.INFO_ICON,
            accentName: SettingsAccent.AMBER,
            isDark: this.isDark
          })

          SettingsSectionTitle({
            title: SettingsText.ABOUT_SECTION_PROJECT,
            subtitle: SettingsText.ABOUT_SECTION_PROJECT_DESC,
            isDark: this.isDark
          })

          Column() {
            this.aboutInfoRow(SettingsText.ABOUT_VERSION_LABEL, this.versionValue)
            this.aboutInfoRow(SettingsText.ABOUT_GITHUB_LABEL, SettingsText.ABOUT_GITHUB_VALUE)
            this.aboutInfoRow(SettingsText.ABOUT_FREERDP_ADAPTATION_LABEL,
              SettingsText.ABOUT_FREERDP_ADAPTATION_VALUE)
            this.aboutInfoRow(SettingsText.ABOUT_XRDP_ADAPTATION_LABEL, SettingsText.ABOUT_XRDP_ADAPTATION_VALUE)
            this.aboutInfoRow(SettingsText.ABOUT_LICENSE_LABEL, SettingsText.ABOUT_LICENSE_VALUE)
          }
          .width('100%')
          .padding(16)
          .backgroundColor(this.cardHovered || this.cardPressed ? SettingsTheme.cardHoverBackground(this.isDark) :
            SettingsTheme.cardBackground(this.isDark))
          .borderRadius(SettingsTheme.CARD_RADIUS)
          .border({
            width: 1,
            color: SettingsTheme.borderColor(this.isDark)
          })
          .shadow(SettingsTheme.shadow(this.isDark, this.cardHovered))
          .translate({ y: this.cardHovered ? -5 : 0 })
          .scale({
            x: this.cardPressed ? 0.99 : this.cardHovered ? 1.012 : 1,
            y: this.cardPressed ? 0.99 : this.cardHovered ? 1.012 : 1
          })
          .margin({ bottom: 10 })
          .onHover((isHover: boolean) => {
            this.setCardHovered(isHover)
          })
          .onTouch((event: TouchEvent) => {
            if (event.type === TouchType.Down) {
              this.cardPressed = true
              return
            }
            if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
              this.cardPressed = false
            }
          })
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
