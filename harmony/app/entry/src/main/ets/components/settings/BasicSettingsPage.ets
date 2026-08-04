import { connection } from '@kit.NetworkKit'
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
  @State private interfaceName: string = SettingsText.EMPTY_VALUE
  @State private ipAddresses: string[] = [SettingsText.NETWORK_LOADING]
  @State private networkCardHovered: boolean = false
  @State private networkCardPressed: boolean = false

  aboutToAppear(): void {
    this.localMode = SettingsTheme.getStoredAppearanceMode()
    this.refreshIp()
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

  private setNetworkCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.networkCardHovered = hovered
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
      this.ipAddresses = [`${SettingsText.NETWORK_READ_FAILED}: ${JSON.stringify(error)}`]
    }
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

  @Builder
  private networkInfoCard() {
    Column() {
      Row() {
        Column() {
          Text(SettingsText.NETWORK_NAME_LABEL)
            .fontSize(12)
            .fontColor(SettingsTheme.mutedText(this.isVisualDark()))
            .width('100%')
            .margin({ bottom: 4 })
          Text(this.interfaceName)
            .fontSize(16)
            .fontWeight(FontWeight.Medium)
            .fontColor(SettingsTheme.primaryText(this.isVisualDark()))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
        }
        .layoutWeight(1)

        Button(SettingsText.NETWORK_REFRESH_ACTION)
          .type(ButtonType.Normal)
          .constraintSize({ minHeight: 48 })
          .borderRadius(SettingsTheme.BUTTON_RADIUS)
          .backgroundColor(SettingsTheme.subtleButton(this.isVisualDark(), false, false))
          .fontColor(SettingsTheme.primaryText(this.isVisualDark()))
          .fontSize(12)
          .stateEffect(false)
          .onClick(() => {
            this.refreshIp()
          })
      }
      .alignItems(VerticalAlign.Center)
      .width('100%')
      .margin({ bottom: 14 })

      Text(SettingsText.NETWORK_IP_LABEL)
        .fontSize(12)
        .fontColor(SettingsTheme.mutedText(this.isVisualDark()))
        .width('100%')
        .margin({ bottom: 8 })

      ForEach(this.ipAddresses, (address: string) => {
        Text(address)
          .fontSize(15)
          .fontColor(SettingsTheme.secondaryText(this.isVisualDark()))
          .width('100%')
          .margin({ bottom: 8 })
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }, (address: string) => address)
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .padding(16)
    .backgroundColor(this.networkCardHovered || this.networkCardPressed ?
      SettingsTheme.cardHoverBackground(this.isVisualDark()) : SettingsTheme.cardBackground(this.isVisualDark()))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isVisualDark())
    })
    .shadow(SettingsTheme.shadow(this.isVisualDark(), this.networkCardHovered))
    .translate({ y: this.networkCardHovered ? -5 : 0 })
    .scale({
      x: this.networkCardPressed ? 0.99 : this.networkCardHovered ? 1.012 : 1,
      y: this.networkCardPressed ? 0.99 : this.networkCardHovered ? 1.012 : 1
    })
    .margin({ bottom: 10 })
    .onHover((isHover: boolean) => {
      this.setNetworkCardHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.networkCardPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.networkCardPressed = false
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

          SettingsSectionTitle({
            title: SettingsText.BASIC_NETWORK_SECTION,
            subtitle: SettingsText.NETWORK_SECTION_DETAIL_DESC,
            isDark: this.isVisualDark()
          })

          this.networkInfoCard()
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
