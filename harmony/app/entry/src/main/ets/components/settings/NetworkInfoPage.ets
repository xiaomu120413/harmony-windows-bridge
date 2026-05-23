import { connection } from '@kit.NetworkKit'
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
export struct NetworkInfoPage {
  onBack: () => void = () => {
  }
  @Prop isDark: boolean = false
  @State private interfaceName: string = SettingsText.EMPTY_VALUE
  @State private ipAddresses: string[] = [SettingsText.NETWORK_LOADING]
  @State private cardHovered: boolean = false
  @State private cardPressed: boolean = false

  aboutToAppear(): void {
    this.refreshIp()
  }

  private setCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.cardHovered = hovered
    })
  }

  private async refreshIp(): Promise<void> {
    this.interfaceName = SettingsText.EMPTY_VALUE
    this.ipAddresses = [SettingsText.NETWORK_LOADING]

    try {
      const netHandle = await connection.getDefaultNet()
      if (!netHandle || netHandle.netId === 0) {
        this.ipAddresses = [SettingsText.NETWORK_EMPTY]
        return
      }

      const properties = await connection.getConnectionProperties(netHandle)
      this.interfaceName = properties.interfaceName || SettingsText.EMPTY_VALUE

      const addresses: string[] = []
      for (let i = 0; i < properties.linkAddresses.length; i++) {
        const linkAddress = properties.linkAddresses[i]
        const address = linkAddress.address.address
        if (!address || address === '::1' || address.indexOf('127.') === 0) {
          continue
        }
        addresses.push(`${address}/${linkAddress.prefixLength}`)
      }

      this.ipAddresses = addresses.length > 0 ? addresses : [SettingsText.NETWORK_NO_IP]
    } catch (error) {
      this.ipAddresses = [`${SettingsText.NETWORK_READ_FAILED}：${JSON.stringify(error)}`]
    }
  }

  build() {
    Column() {
      SettingsPageHeader({
        title: SettingsText.NETWORK_TITLE,
        subtitle: SettingsText.NETWORK_SUBTITLE,
        isDark: this.isDark,
        onBack: this.onBack
      })

      Scroll() {
        Column() {
          SettingsFeatureCard({
            title: SettingsText.NETWORK_OVERVIEW_TITLE,
            body: SettingsText.NETWORK_OVERVIEW_BODY,
            meta: SettingsText.NETWORK_OVERVIEW_META,
            iconResource: SettingsResources.NETWORK_ICON,
            accentName: SettingsAccent.GREEN,
            isDark: this.isDark
          })

          SettingsSectionTitle({
            title: SettingsText.NETWORK_SECTION_DETAIL,
            subtitle: SettingsText.NETWORK_SECTION_DETAIL_DESC,
            isDark: this.isDark
          })

          Column() {
            Text(SettingsText.NETWORK_NAME_LABEL)
              .fontSize(13)
              .fontColor(SettingsTheme.mutedText(this.isDark))
              .width('100%')
              .margin({ bottom: 4 })
            Text(this.interfaceName)
              .fontSize(18)
              .fontWeight(FontWeight.Medium)
              .fontColor(SettingsTheme.primaryText(this.isDark))
              .width('100%')
              .margin({ bottom: 14 })

            Text(SettingsText.NETWORK_IP_LABEL)
              .fontSize(13)
              .fontColor(SettingsTheme.mutedText(this.isDark))
              .width('100%')
              .margin({ bottom: 8 })

            ForEach(this.ipAddresses, (address: string) => {
              Text(address)
                .fontSize(16)
                .fontColor(SettingsTheme.secondaryText(this.isDark))
                .width('100%')
                .margin({ bottom: 8 })
            }, (address: string) => address)
          }
          .alignItems(HorizontalAlign.Start)
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
