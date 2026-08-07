import { SettingsRemoteControlSection, SettingsText } from './SettingsConstants'
import { SettingsAccent, SettingsAccentName, SettingsResources, SettingsTheme } from './SettingsUi'
import { LayoutMode } from '../../adaptive/WindowLayoutPolicy'

@Component
export struct XrdpServerCard {
  @Prop isDark: boolean = false
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  @Prop targetSection: string = ''
  @Prop localIpAddress: string = SettingsText.NETWORK_LOADING
  @Prop connectionCopyLabel: string = SettingsText.REMOTE_CONNECTION_COPY_ACTION
  @Prop running: boolean = false
  @Prop busy: boolean = false
  @Prop state: string = 'Stopped'
  @Prop port: number = 3390
  @Prop message: string = ''
  onRefresh: () => void = () => {
  }
  onStart: () => void = () => {
  }
  onCopyConnectionAddress: () => void = () => {
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
    return SettingsText.remoteServerStatusText(this.running, this.busy, this.state, this.port)
  }

  private actionText(): string {
    if (this.busy) {
      return SettingsText.REMOTE_SERVER_BUSY
    }
    return this.running ? SettingsText.REMOTE_SERVER_REFRESH_ACTION : SettingsText.REMOTE_SERVER_START_ACTION
  }

  private actionDisabled(): boolean {
    return this.busy
  }

  private connectionAddress(): string {
    if (this.localIpAddress === SettingsText.NETWORK_LOADING ||
      this.localIpAddress === SettingsText.NETWORK_EMPTY ||
      this.localIpAddress === SettingsText.NETWORK_NO_IP ||
      this.localIpAddress === SettingsText.NETWORK_READ_FAILED) {
      return SettingsText.REMOTE_CONNECTION_ADDRESS_WAITING
    }
    const host = this.localIpAddress.indexOf(':') >= 0 ? `[${this.localIpAddress}]` : this.localIpAddress
    const safePort = this.port > 0 ? this.port : 3390
    return `${host}:${safePort}`
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
  private addressBlock() {
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
          Text(SettingsText.REMOTE_SERVER_TITLE)
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .width('100%')
          Text(SettingsText.REMOTE_SERVER_DESC)
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

        Button(this.actionText())
          .type(ButtonType.Normal)
          .constraintSize({ minHeight: 48 })
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
      }
      .alignItems(VerticalAlign.Center)
      .width('100%')

      if (this.layoutMode === LayoutMode.EXPANDED) {
        Row({ space: 14 }) {
          this.addressBlock()
          this.copyButton()
        }
        .alignItems(VerticalAlign.Center)
        .width('100%')
      } else {
        Column({ space: 12 }) {
          this.addressBlock()
          this.copyButton()
        }
        .alignItems(HorizontalAlign.Start)
        .width('100%')
      }

      Text(this.message.length > 0 ? this.message : SettingsText.REMOTE_CONNECTION_NETWORK_HINT)
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
export struct RemoteFilesCard {
  @Prop isDark: boolean = false
  @Prop targetSection: string = ''
  onOpenRemoteFilesDirectory: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private isTarget(): boolean {
    return this.targetSection === SettingsRemoteControlSection.FILES
  }

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  build() {
    Row({ space: 14 }) {
      Column() {
        Image(SettingsResources.REMOTE_FILES_ICON)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.PURPLE))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(SettingsTheme.accentBackground(this.isDark, SettingsAccent.PURPLE))

      Column({ space: 5 }) {
        Text(SettingsText.REMOTE_FILES_FEATURE_TITLE)
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
        Text(SettingsText.REMOTE_FILES_FEATURE_DESC)
          .fontSize(13)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .lineHeight(19)
          .width('100%')
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .layoutWeight(1)

      Button(SettingsText.REMOTE_FILES_FEATURE_ACTION)
        .type(ButtonType.Normal)
        .constraintSize({ minHeight: 48 })
        .borderRadius(SettingsTheme.BUTTON_RADIUS)
        .backgroundColor(SettingsTheme.subtleButton(this.isDark, false, false))
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .fontSize(12)
        .stateEffect(false)
        .onClick(() => {
          this.onOpenRemoteFilesDirectory()
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
      color: this.isTarget() ? SettingsTheme.accentColor(this.isDark, SettingsAccent.PURPLE) :
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
    return this.gateEnabled ? SettingsText.REMOTE_ACCESS_GATE_ON : SettingsText.REMOTE_ACCESS_GATE_OFF
  }

  private accessCodeText(): string {
    return this.gateEnabled ? this.accessCode : SettingsText.REMOTE_ACCESS_CODE_DISABLED
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
          Text(SettingsText.REMOTE_ACCESS_GATE_TITLE)
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .width('100%')
          Text(SettingsText.REMOTE_ACCESS_GATE_DESC)
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
          Text(SettingsText.REMOTE_ACCESS_CODE_LABEL)
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

        Button(SettingsText.REMOTE_ACCESS_CODE_ACTION)
          .type(ButtonType.Normal)
          .constraintSize({ minHeight: 48 })
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
  @Prop title: string = SettingsText.REMOTE_SCREEN_PERMISSION_TITLE
  @Prop description: string = SettingsText.REMOTE_SCREEN_PERMISSION_DESC
  @Prop grantedText: string = SettingsText.REMOTE_PERMISSION_GRANTED
  @Prop missingText: string = SettingsText.REMOTE_PERMISSION_MISSING
  @Prop actionTextValue: string = SettingsText.REMOTE_PERMISSION_ACTION
  @Prop busyText: string = SettingsText.REMOTE_PERMISSION_BUSY
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
        .constraintSize({ minHeight: 48 })
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
