import { LayoutMode } from '../../adaptive/WindowLayoutPolicy'
import {
  SettingsAccent,
  SettingsInfoCard,
  SettingsListItem,
  SettingsPageHeader,
  SettingsResources,
  SettingsSectionTitle,
  SettingsTheme
} from './SettingsUi'
import { SettingsText } from './SettingsConstants'

@Component
export struct ProjectHelpPage {
  onBack: () => void = () => {
  }
  onCopyConnectionAddress: () => void = () => {
  }
  onOpenRemoteControlSettings: () => void = () => {
  }
  @Prop isDark: boolean = false
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  @Prop remoteControlServerAvailable: boolean = false
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerPort: number = 3390
  @Prop localIpAddress: string = SettingsText.NETWORK_LOADING
  @Prop connectionCopyLabel: string = SettingsText.REMOTE_CONNECTION_COPY_ACTION
  @State private showControllerGuide: boolean = false
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

  private connectionAddress(): string {
    if (this.localIpAddress === SettingsText.NETWORK_LOADING ||
      this.localIpAddress === SettingsText.NETWORK_EMPTY ||
      this.localIpAddress === SettingsText.NETWORK_NO_IP ||
      this.localIpAddress === SettingsText.NETWORK_READ_FAILED) {
      return SettingsText.REMOTE_CONNECTION_ADDRESS_WAITING
    }
    const host = this.localIpAddress.indexOf(':') >= 0 ? `[${this.localIpAddress}]` : this.localIpAddress
    const port = this.xrdpServerPort > 0 ? this.xrdpServerPort : 3390
    return `${host}:${port}`
  }

  private copyDisabled(): boolean {
    return this.connectionAddress() === SettingsText.REMOTE_CONNECTION_ADDRESS_WAITING
  }

  @Builder
  private copyButton() {
    Button() {
      Row({ space: 6 }) {
        Image(SettingsResources.COPY_ICON)
          .width(16)
          .height(16)
          .fillColor(this.copyDisabled() ? SettingsTheme.disabledText(this.isDark) :
            SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE))
          .draggable(false)
        Text(this.connectionCopyLabel)
          .fontSize(12)
          .fontWeight(FontWeight.Medium)
      }
      .alignItems(VerticalAlign.Center)
    }
    .type(ButtonType.Normal)
    .constraintSize({ minHeight: 48 })
    .padding({ left: 14, right: 14 })
    .borderRadius(SettingsTheme.BUTTON_RADIUS)
    .backgroundColor(this.copyDisabled() ? SettingsTheme.disabledButton(this.isDark) :
      SettingsTheme.accentBackground(this.isDark, SettingsAccent.BLUE))
    .fontColor(this.copyDisabled() ? SettingsTheme.disabledText(this.isDark) :
      SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE))
    .stateEffect(false)
    .enabled(!this.copyDisabled())
    .onClick(() => {
      if (!this.copyDisabled()) {
        this.onCopyConnectionAddress()
      }
    })
  }

  @Builder
  private addressValue() {
    Column({ space: 6 }) {
      Text(SettingsText.REMOTE_CONNECTION_ADDRESS_LABEL)
        .fontSize(12)
        .fontColor(SettingsTheme.mutedText(this.isDark))
        .width('100%')
      Text(this.connectionAddress())
        .fontSize(this.layoutMode === LayoutMode.EXPANDED ? 22 : 18)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(2)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .width('100%')
    }
    .alignItems(HorizontalAlign.Start)
    .layoutWeight(this.layoutMode === LayoutMode.EXPANDED ? 1 : 0)
    .width('100%')
  }

  @Builder
  private controlledReadyCard() {
    Column({ space: 16 }) {
      Row({ space: 14 }) {
        Column() {
          Image(SettingsResources.REMOTE_SERVICE_ICON)
            .width(SettingsTheme.ICON_SIZE)
            .height(SettingsTheme.ICON_SIZE)
            .fillColor(SettingsTheme.accentColor(this.isDark, this.xrdpServerRunning ? SettingsAccent.GREEN :
              SettingsAccent.AMBER))
            .draggable(false)
        }
        .width(SettingsTheme.ICON_TILE_SIZE)
        .height(SettingsTheme.ICON_TILE_SIZE)
        .justifyContent(FlexAlign.Center)
        .alignItems(HorizontalAlign.Center)
        .borderRadius(SettingsTheme.BUTTON_RADIUS)
        .backgroundColor(SettingsTheme.accentBackground(this.isDark, this.xrdpServerRunning ? SettingsAccent.GREEN :
          SettingsAccent.AMBER))

        Column({ space: 5 }) {
          Text(this.xrdpServerRunning ? SettingsText.CONTROLLED_HELP_READY :
            SettingsText.CONTROLLED_HELP_NOT_READY)
            .fontSize(16)
            .fontWeight(FontWeight.Bold)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .width('100%')
          Text(this.xrdpServerRunning ? SettingsText.CONTROLLED_HELP_READY_DESC :
            SettingsText.CONTROLLED_HELP_NOT_READY_DESC)
            .fontSize(13)
            .fontColor(SettingsTheme.mutedText(this.isDark))
            .lineHeight(19)
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
        }
        .alignItems(HorizontalAlign.Start)
        .layoutWeight(1)

        Button(SettingsText.CONTROLLED_HELP_SETTINGS_ACTION)
          .type(ButtonType.Normal)
          .constraintSize({ minHeight: 48 })
          .padding({ left: 14, right: 14 })
          .borderRadius(SettingsTheme.BUTTON_RADIUS)
          .backgroundColor(SettingsTheme.subtleButton(this.isDark, false, false))
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .fontSize(12)
          .stateEffect(false)
          .onClick(() => {
            this.onOpenRemoteControlSettings()
          })
      }
      .alignItems(VerticalAlign.Center)
      .width('100%')

      if (this.layoutMode === LayoutMode.EXPANDED) {
        Row({ space: 14 }) {
          this.addressValue()
          this.copyButton()
        }
        .alignItems(VerticalAlign.Center)
        .width('100%')
      } else {
        Column({ space: 12 }) {
          this.addressValue()
          this.copyButton()
        }
        .alignItems(HorizontalAlign.Start)
        .width('100%')
      }
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .padding(18)
    .backgroundColor(SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .margin({ bottom: 14 })
  }

  @Builder
  private controlStep(number: string, title: string, body: string) {
    Row({ space: 12 }) {
      Column() {
        Text(number)
          .fontSize(12)
          .fontWeight(FontWeight.Bold)
          .fontColor(Color.White)
      }
      .width(28)
      .height(28)
      .borderRadius(14)
      .backgroundColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE))
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)

      Column({ space: 6 }) {
        Text(title)
          .fontSize(14)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
        Text(body)
          .fontSize(12)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .lineHeight(18)
          .width('100%')
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)
    }
    .alignItems(VerticalAlign.Top)
    .layoutWeight(this.layoutMode === LayoutMode.EXPANDED ? 1 : 0)
    .width('100%')
    .padding(14)
    .backgroundColor(SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
  }

  @Builder
  private controlledGuide() {
    SettingsSectionTitle({
      title: SettingsText.CONTROLLED_HELP_TITLE,
      subtitle: SettingsText.CONTROLLED_HELP_DESC,
      isDark: this.isDark
    })
    this.controlledReadyCard()
    SettingsSectionTitle({
      title: SettingsText.CONTROLLED_HELP_STEPS_TITLE,
      subtitle: SettingsText.REMOTE_CONNECTION_NETWORK_HINT,
      isDark: this.isDark
    })
    if (this.layoutMode === LayoutMode.EXPANDED) {
      Row({ space: 12 }) {
        this.controlStep('1', SettingsText.CONTROLLED_HELP_STEP_ONE, SettingsText.CONTROLLED_HELP_STEP_ONE_DESC)
        this.controlStep('2', SettingsText.CONTROLLED_HELP_STEP_TWO, SettingsText.CONTROLLED_HELP_STEP_TWO_DESC)
        this.controlStep('3', SettingsText.CONTROLLED_HELP_STEP_THREE, SettingsText.CONTROLLED_HELP_STEP_THREE_DESC)
      }
      .alignItems(VerticalAlign.Top)
      .width('100%')
      .margin({ bottom: 12 })
    } else {
      Column({ space: 10 }) {
        this.controlStep('1', SettingsText.CONTROLLED_HELP_STEP_ONE, SettingsText.CONTROLLED_HELP_STEP_ONE_DESC)
        this.controlStep('2', SettingsText.CONTROLLED_HELP_STEP_TWO, SettingsText.CONTROLLED_HELP_STEP_TWO_DESC)
        this.controlStep('3', SettingsText.CONTROLLED_HELP_STEP_THREE, SettingsText.CONTROLLED_HELP_STEP_THREE_DESC)
      }
      .width('100%')
      .margin({ bottom: 12 })
    }
    SettingsInfoCard({
      title: SettingsText.USAGE_TROUBLESHOOT_TITLE,
      body: `${SettingsText.CONTROLLED_HELP_SECURITY_NOTE}\n${SettingsText.CONTROLLED_HELP_TROUBLESHOOT}`,
      isDark: this.isDark
    })
  }

  @Builder
  private controllerGuide() {
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
      title: SettingsText.USAGE_USERNAME_TITLE,
      body: SettingsText.USAGE_USERNAME_BODY,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsText.USAGE_HARDWARE_ACCEL_TITLE,
      body: SettingsText.USAGE_HARDWARE_ACCEL_BODY,
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
          if (this.remoteControlServerAvailable) {
            this.controlledGuide()
            SettingsListItem({
              title: SettingsText.CONTROLLER_HELP_TITLE,
              description: SettingsText.CONTROLLER_HELP_DESC,
              iconResource: SettingsResources.REMOTE_CONTROL_ICON,
              accentName: SettingsAccent.CYAN,
              selected: this.showControllerGuide,
              isDark: this.isDark,
              onPress: (): void => {
                this.showControllerGuide = !this.showControllerGuide
              }
            })
            if (this.showControllerGuide) {
              this.controllerGuide()
            }
          } else {
            this.controllerGuide()
          }

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
