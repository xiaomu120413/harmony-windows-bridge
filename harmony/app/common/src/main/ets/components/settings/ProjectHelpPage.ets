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
import {
  SettingsAboutText,
  SettingsHelpText,
  SettingsRemoteText,
  SettingsShellText,
  SettingsUsageText
} from './SettingsConstants'

@Component
struct ControlledHelpStatusCard {
  @Prop isDark: boolean = false
  @Prop running: boolean = false
  onOpenRemoteControlSettings: () => void = () => {
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
        Image(SettingsResources.REMOTE_SERVICE_ICON)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(SettingsTheme.accentColor(this.isDark, this.running ? SettingsAccent.GREEN :
            SettingsAccent.AMBER))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(SettingsTheme.accentBackground(this.isDark, this.running ? SettingsAccent.GREEN :
        SettingsAccent.AMBER))

      Column({ space: 5 }) {
        Text(this.running ? SettingsHelpText.CONTROLLED_READY : SettingsHelpText.CONTROLLED_NOT_READY)
          .fontSize(16)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
        Text(this.running ? SettingsHelpText.CONTROLLED_READY_DESC : SettingsHelpText.CONTROLLED_NOT_READY_DESC)
          .fontSize(13)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .lineHeight(19)
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)

      Button(SettingsHelpText.CONTROLLED_SETTINGS_ACTION)
        .type(ButtonType.Normal)
        .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
        .padding({ left: 14, right: 14 })
        .borderRadius(SettingsTheme.BUTTON_RADIUS)
        .backgroundColor(SettingsTheme.subtleButton(this.isDark, this.hovered, this.pressed))
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .fontSize(12)
        .stateEffect(false)
        .onClick(() => {
          this.onOpenRemoteControlSettings()
        })
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding(18)
    .backgroundColor(this.hovered || this.pressed ? SettingsTheme.cardHoverBackground(this.isDark) :
      SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({ width: 1, color: SettingsTheme.borderColor(this.isDark) })
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
struct ControlledHelpStepCard {
  @Prop isDark: boolean = false
  @Prop expanded: boolean = false
  @Prop number: string = ''
  @Prop title: string = ''
  @Prop body: string = ''
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  build() {
    Row({ space: 12 }) {
      Column() {
        Text(this.number)
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
        Text(this.title)
          .fontSize(14)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
        Text(this.body)
          .fontSize(12)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .lineHeight(18)
          .width('100%')
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)
    }
    .alignItems(VerticalAlign.Top)
    .layoutWeight(this.expanded ? 1 : 0)
    .width('100%')
    .padding(14)
    .backgroundColor(this.hovered || this.pressed ? SettingsTheme.cardHoverBackground(this.isDark) :
      SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({ width: 1, color: SettingsTheme.borderColor(this.isDark) })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered))
    .translate({ y: this.hovered ? -5 : 0 })
    .scale({
      x: this.pressed ? 0.99 : this.hovered ? 1.012 : 1,
      y: this.pressed ? 0.99 : this.hovered ? 1.012 : 1
    })
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
export struct ProjectHelpPage {
  onBack: () => void = () => {
  }
  onOpenRemoteControlSettings: () => void = () => {
  }
  @Prop isDark: boolean = false
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  @Prop remoteControlServerAvailable: boolean = false
  @Prop xrdpServerRunning: boolean = false
  @State private showControllerGuide: boolean = false
  @State private aboutCardHovered: boolean = false
  @State private aboutCardPressed: boolean = false

  aboutToAppear(): void {
    this.showControllerGuide = !this.remoteControlServerAvailable
  }

  private setAboutCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.aboutCardHovered = hovered
    })
  }

  private isUrl(value: string): boolean {
    return value.startsWith('http://') || value.startsWith('https://')
  }

  @Builder
  private controlledGuide() {
    SettingsSectionTitle({
      title: SettingsRemoteText.CONTROLLED_DEVICE_TITLE,
      subtitle: SettingsHelpText.CONTROLLED_DESC,
      isDark: this.isDark
    })
    ControlledHelpStatusCard({
      isDark: this.isDark,
      running: this.xrdpServerRunning,
      onOpenRemoteControlSettings: this.onOpenRemoteControlSettings
    })
    SettingsSectionTitle({
      title: SettingsHelpText.CONTROLLED_STEPS_TITLE,
      subtitle: SettingsRemoteText.CONNECTION_NETWORK_HINT,
      isDark: this.isDark
    })
    if (this.layoutMode === LayoutMode.EXPANDED) {
      Row({ space: 12 }) {
        ControlledHelpStepCard({
          isDark: this.isDark,
          expanded: true,
          number: '1',
          title: SettingsHelpText.CONTROLLED_STEP_ONE,
          body: SettingsHelpText.CONTROLLED_STEP_ONE_DESC
        })
        ControlledHelpStepCard({
          isDark: this.isDark,
          expanded: true,
          number: '2',
          title: SettingsHelpText.CONTROLLED_STEP_TWO,
          body: SettingsHelpText.CONTROLLED_STEP_TWO_DESC
        })
        ControlledHelpStepCard({
          isDark: this.isDark,
          expanded: true,
          number: '3',
          title: SettingsHelpText.CONTROLLED_STEP_THREE,
          body: SettingsHelpText.CONTROLLED_STEP_THREE_DESC
        })
      }
      .alignItems(VerticalAlign.Top)
      .width('100%')
      .margin({ bottom: 12 })
    } else {
      Column({ space: 10 }) {
        ControlledHelpStepCard({
          isDark: this.isDark,
          number: '1',
          title: SettingsHelpText.CONTROLLED_STEP_ONE,
          body: SettingsHelpText.CONTROLLED_STEP_ONE_DESC
        })
        ControlledHelpStepCard({
          isDark: this.isDark,
          number: '2',
          title: SettingsHelpText.CONTROLLED_STEP_TWO,
          body: SettingsHelpText.CONTROLLED_STEP_TWO_DESC
        })
        ControlledHelpStepCard({
          isDark: this.isDark,
          number: '3',
          title: SettingsHelpText.CONTROLLED_STEP_THREE,
          body: SettingsHelpText.CONTROLLED_STEP_THREE_DESC
        })
      }
      .width('100%')
      .margin({ bottom: 12 })
    }
    SettingsInfoCard({
      title: SettingsUsageText.TROUBLESHOOT_TITLE,
      body: SettingsHelpText.CONTROLLED_TROUBLESHOOT,
      isDark: this.isDark
    })
  }

  @Builder
  private controllerGuide() {
    SettingsSectionTitle({
      title: SettingsUsageText.PREP_SECTION,
      subtitle: SettingsUsageText.PREP_SECTION_DESC,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsUsageText.PREP_TITLE,
      body: SettingsUsageText.PREP_BODY,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsUsageText.WINDOWS_RDP_TITLE,
      body: SettingsUsageText.WINDOWS_RDP_BODY,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsUsageText.USERNAME_TITLE,
      body: SettingsUsageText.USERNAME_BODY,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsUsageText.HARDWARE_ACCEL_TITLE,
      body: SettingsUsageText.HARDWARE_ACCEL_BODY,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsUsageText.FORM_TITLE,
      body: SettingsUsageText.FORM_BODY,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsRemoteText.FILES_TITLE,
      body: SettingsRemoteText.FILES_DESC,
      isDark: this.isDark
    })
    SettingsSectionTitle({
      title: SettingsUsageText.SECURITY_SECTION,
      subtitle: SettingsUsageText.SECURITY_SECTION_DESC,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsUsageText.CERT_TITLE,
      body: SettingsUsageText.CERT_BODY,
      isDark: this.isDark
    })
    SettingsInfoCard({
      title: SettingsUsageText.TROUBLESHOOT_TITLE,
      body: SettingsUsageText.TROUBLESHOOT_BODY,
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
      this.aboutInfoRow(SettingsAboutText.GITHUB_LABEL, SettingsAboutText.GITHUB_VALUE)
      this.aboutInfoRow(SettingsAboutText.FREERDP_LABEL, SettingsAboutText.FREERDP_VALUE)
      this.aboutInfoRow(SettingsAboutText.XRDP_LABEL, SettingsAboutText.XRDP_VALUE)
      this.aboutInfoRow(SettingsAboutText.LICENSE_LABEL, SettingsAboutText.LICENSE_VALUE)
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
        title: SettingsShellText.HELP_TITLE,
        subtitle: this.remoteControlServerAvailable ? SettingsHelpText.PAGE_SUBTITLE :
          SettingsShellText.HELP_CLIENT_DESC,
        isDark: this.isDark,
        onBack: this.onBack
      })

      Scroll() {
        Column() {
          if (this.remoteControlServerAvailable) {
            this.controlledGuide()
          }
          SettingsListItem({
            title: SettingsHelpText.CONTROLLER_TITLE,
            description: SettingsHelpText.CONTROLLER_DESC,
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

          SettingsSectionTitle({
            title: SettingsHelpText.ABOUT_SECTION,
            subtitle: SettingsAboutText.SECTION_DESC,
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
