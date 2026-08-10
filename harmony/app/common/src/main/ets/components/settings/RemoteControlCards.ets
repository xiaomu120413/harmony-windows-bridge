import { SettingsNetworkText, SettingsRemoteControlSection, SettingsRemoteText } from './SettingsConstants'
import { SettingsAccent, SettingsAccentName, SettingsResources, SettingsTheme } from './SettingsUi'
import { LayoutMode } from '../../adaptive/WindowLayoutPolicy'

@Component
export struct XrdpServerCard {
  @Prop isDark: boolean = false
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  @Prop targetSection: string = ''
  @Prop localIpAddress: string = SettingsNetworkText.LOADING
  @Prop running: boolean = false
  @Prop busy: boolean = false
  @Prop state: string = 'Stopped'
  @Prop port: number = 3390
  @Prop message: string = ''
  onRefresh: () => void = () => {
  }
  onStart: () => void = () => {
  }
  onStop: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private isTarget(): boolean {
    return this.targetSection === SettingsRemoteControlSection.SERVER
  }

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  private statusText(): string {
    return SettingsRemoteText.serverStatusText(this.running, this.busy, this.state, this.port)
  }

  private actionText(): string {
    if (this.busy) {
      return SettingsRemoteText.SERVER_BUSY
    }
    return this.running ? SettingsRemoteText.SERVER_REFRESH_ACTION : SettingsRemoteText.SERVER_START_ACTION
  }

  private actionDisabled(): boolean {
    return this.busy
  }

  private connectionAddress(): string {
    if (this.localIpAddress === SettingsNetworkText.LOADING ||
      this.localIpAddress === SettingsNetworkText.EMPTY ||
      this.localIpAddress === SettingsNetworkText.NO_IP ||
      this.localIpAddress === SettingsNetworkText.READ_FAILED) {
      return SettingsRemoteText.CONNECTION_ADDRESS_WAITING
    }
    const host = this.localIpAddress.indexOf(':') >= 0 ? `[${this.localIpAddress}]` : this.localIpAddress
    const safePort = this.port > 0 ? this.port : 3390
    return `${host}:${safePort}`
  }

  @Builder
  private addressBlock() {
    Column({ space: 6 }) {
      Text(SettingsRemoteText.CONNECTION_ADDRESS_LABEL)
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
    .width('100%')
  }

  build() {
    Column({ space: 16 }) {
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
          Text(SettingsRemoteText.CONTROLLED_DEVICE_TITLE)
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .width('100%')
          Text(SettingsRemoteText.SERVER_DESC)
            .fontSize(13)
            .fontColor(SettingsTheme.mutedText(this.isDark))
            .lineHeight(19)
            .width('100%')
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
          Text(this.statusText())
            .fontSize(12)
            .fontColor(SettingsTheme.accentColor(this.isDark, this.running ? SettingsAccent.GREEN :
              SettingsAccent.AMBER))
            .width('100%')
        }
        .layoutWeight(1)

        Row({ space: 8 }) {
          Button(this.actionText())
            .type(ButtonType.Normal)
            .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
            .borderRadius(SettingsTheme.BUTTON_RADIUS)
            .backgroundColor(this.actionDisabled() ? SettingsTheme.disabledButton(this.isDark) :
              SettingsTheme.subtleButton(this.isDark, false, false))
            .fontColor(this.actionDisabled() ? SettingsTheme.disabledText(this.isDark) :
              SettingsTheme.primaryText(this.isDark))
            .fontSize(12)
            .stateEffect(false)
            .enabled(!this.actionDisabled())
            .onClick(() => {
              if (this.actionDisabled()) {
                return
              }
              if (this.running) {
                this.onRefresh()
                return
              }
              this.onStart()
            })
          if (this.running) {
            Button(SettingsRemoteText.SERVER_STOP_ACTION)
              .type(ButtonType.Normal)
              .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
              .borderRadius(SettingsTheme.BUTTON_RADIUS)
              .backgroundColor(this.actionDisabled() ? SettingsTheme.disabledButton(this.isDark) :
                SettingsTheme.accentBackground(this.isDark, SettingsAccent.ORANGE))
              .fontColor(this.actionDisabled() ? SettingsTheme.disabledText(this.isDark) :
                SettingsTheme.accentColor(this.isDark, SettingsAccent.ORANGE))
              .fontSize(12)
              .stateEffect(false)
              .enabled(!this.actionDisabled())
              .onClick(() => {
                if (!this.actionDisabled()) {
                  this.onStop()
                }
              })
          }
        }
      }
      .alignItems(VerticalAlign.Center)
      .width('100%')

      this.addressBlock()

      Text(this.message.length > 0 ? this.message : SettingsRemoteText.CONNECTION_NETWORK_HINT)
        .fontSize(12)
        .fontColor(SettingsTheme.mutedText(this.isDark))
        .lineHeight(18)
        .width('100%')
        .maxLines(2)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .padding(16)
    .backgroundColor(this.hovered || this.pressed || this.isTarget() ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: this.isTarget() ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) :
        SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered || this.isTarget()))
    .translate({ y: this.hovered ? -5 : 0 })
    .scale({
      x: this.pressed ? 0.99 : this.hovered ? 1.012 : 1,
      y: this.pressed ? 0.99 : this.hovered ? 1.012 : 1
    })
    .margin({ bottom: 12 })
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
export struct RemoteAccessCard {
  @Prop isDark: boolean = false
  @Prop targetSection: string = ''
  @Prop gateEnabled: boolean = false
  @Prop accessCode: string = '000000'
  onGateChange: (enabled: boolean) => void = (_enabled: boolean) => {
  }
  onRegenerate: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private isTarget(): boolean {
    return this.targetSection === SettingsRemoteControlSection.ACCESS
  }

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  private gateStatusText(): string {
    return this.gateEnabled ? SettingsRemoteText.ACCESS_GATE_ON : SettingsRemoteText.ACCESS_GATE_OFF
  }

  private accessCodeText(): string {
    return this.gateEnabled ? this.accessCode : SettingsRemoteText.ACCESS_GATE_OFF
  }

  private regenerateDisabled(): boolean {
    return !this.gateEnabled
  }

  build() {
    Column({ space: 14 }) {
      Row({ space: 14 }) {
        Column() {
          Image(SettingsResources.REMOTE_ACCESS_ICON)
            .width(SettingsTheme.ICON_SIZE)
            .height(SettingsTheme.ICON_SIZE)
            .fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE))
            .draggable(false)
        }
        .width(SettingsTheme.ICON_TILE_SIZE)
        .height(SettingsTheme.ICON_TILE_SIZE)
        .justifyContent(FlexAlign.Center)
        .alignItems(HorizontalAlign.Center)
        .borderRadius(SettingsTheme.BUTTON_RADIUS)
        .backgroundColor(SettingsTheme.accentBackground(this.isDark, SettingsAccent.BLUE))

        Column({ space: 5 }) {
          Text(SettingsRemoteText.ACCESS_TITLE)
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .width('100%')
          Text(SettingsRemoteText.ACCESS_DESC)
            .fontSize(13)
            .fontColor(SettingsTheme.mutedText(this.isDark))
            .lineHeight(19)
            .width('100%')
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
        .layoutWeight(1)

        Toggle({ type: ToggleType.Switch, isOn: this.gateEnabled })
          .onChange((enabled: boolean) => {
            this.onGateChange(enabled)
          })
      }
      .width('100%')
      .alignItems(VerticalAlign.Center)

      Row() {
        Column({ space: 4 }) {
          Text(SettingsRemoteText.ACCESS_CODE_LABEL)
            .fontSize(12)
            .fontColor(SettingsTheme.mutedText(this.isDark))
            .width('100%')
          Text(this.accessCodeText())
            .fontSize(18)
            .fontWeight(FontWeight.Medium)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .width('100%')
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
        .layoutWeight(1)

        Button(SettingsRemoteText.ACCESS_CODE_ACTION)
          .type(ButtonType.Normal)
          .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
          .borderRadius(SettingsTheme.BUTTON_RADIUS)
          .backgroundColor(this.regenerateDisabled() ? SettingsTheme.disabledButton(this.isDark) :
            SettingsTheme.subtleButton(this.isDark, false, false))
          .fontColor(this.regenerateDisabled() ? SettingsTheme.disabledText(this.isDark) :
            SettingsTheme.primaryText(this.isDark))
          .fontSize(12)
          .stateEffect(false)
          .enabled(!this.regenerateDisabled())
          .onClick(() => {
            if (this.regenerateDisabled()) {
              return
            }
            this.onRegenerate()
          })
      }
      .alignItems(VerticalAlign.Center)
      .width('100%')

      Text(this.gateStatusText())
        .fontSize(12)
        .fontColor(SettingsTheme.accentColor(this.isDark, this.gateEnabled ? SettingsAccent.GREEN :
          SettingsAccent.AMBER))
        .width('100%')
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .padding(16)
    .backgroundColor(this.hovered || this.pressed || this.isTarget() ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: this.isTarget() ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) :
        SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered || this.isTarget()))
    .translate({ y: this.hovered ? -5 : 0 })
    .scale({
      x: this.pressed ? 0.99 : this.hovered ? 1.012 : 1,
      y: this.pressed ? 0.99 : this.hovered ? 1.012 : 1
    })
    .margin({ bottom: 12 })
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
export struct RemotePermissionCard {
  @Prop isDark: boolean = false
  @Prop targetSection: string = ''
  @Prop section: string = SettingsRemoteControlSection.SCREEN
  @Prop title: string = SettingsRemoteText.SCREEN_PERMISSION_TITLE
  @Prop description: string = SettingsRemoteText.SCREEN_PERMISSION_DESC
  @Prop grantedText: string = SettingsRemoteText.PERMISSION_GRANTED
  @Prop missingText: string = SettingsRemoteText.PERMISSION_MISSING
  @Prop actionTextValue: string = SettingsRemoteText.PERMISSION_ACTION
  @Prop busyText: string = SettingsRemoteText.PERMISSION_BUSY
  @Prop icon: Resource = SettingsResources.REMOTE_SCREEN_ICON
  @Prop accent: SettingsAccentName = SettingsAccent.CYAN
  @Prop granted: boolean = false
  @Prop busy: boolean = false
  onRequest: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private isTarget(): boolean {
    return this.targetSection === this.section
  }

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  private statusText(): string {
    if (this.busy) {
      return this.busyText
    }
    return this.granted ? this.grantedText : this.missingText
  }

  private actionText(): string {
    if (this.busy) {
      return this.busyText
    }
    return this.granted ? this.grantedText : this.actionTextValue
  }

  private actionDisabled(): boolean {
    return this.busy || this.granted
  }

  build() {
    Row({ space: 14 }) {
      Column() {
        Image(this.icon)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(SettingsTheme.accentColor(this.isDark, this.accent))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(SettingsTheme.accentBackground(this.isDark, this.accent))

      Column({ space: 5 }) {
        Text(this.title)
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
        Text(this.description)
          .fontSize(13)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .lineHeight(19)
          .width('100%')
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        Text(this.statusText())
          .fontSize(12)
          .fontColor(SettingsTheme.accentColor(this.isDark, this.granted ? SettingsAccent.GREEN :
            SettingsAccent.AMBER))
          .width('100%')
      }
      .layoutWeight(1)

      Button(this.actionText())
        .type(ButtonType.Normal)
        .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
        .borderRadius(SettingsTheme.BUTTON_RADIUS)
        .backgroundColor(this.actionDisabled() ? SettingsTheme.disabledButton(this.isDark) :
          SettingsTheme.subtleButton(this.isDark, false, false))
        .fontColor(this.actionDisabled() ? SettingsTheme.disabledText(this.isDark) :
          SettingsTheme.primaryText(this.isDark))
        .fontSize(12)
        .stateEffect(false)
        .enabled(!this.actionDisabled())
        .onClick(() => {
          if (this.actionDisabled()) {
            return
          }
          this.onRequest()
        })
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding(16)
    .backgroundColor(this.hovered || this.pressed || this.isTarget() ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: this.isTarget() ? SettingsTheme.accentColor(this.isDark, this.accent) :
        SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered || this.isTarget()))
    .translate({ y: this.hovered ? -5 : 0 })
    .scale({
      x: this.pressed ? 0.99 : this.hovered ? 1.012 : 1,
      y: this.pressed ? 0.99 : this.hovered ? 1.012 : 1
    })
    .margin({ bottom: 10 })
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
