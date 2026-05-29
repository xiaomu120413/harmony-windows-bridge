import { SettingsRemoteControlSection, SettingsText } from './SettingsConstants'
import { SettingsAccent, SettingsResources, SettingsTheme } from './SettingsUi'

@Component
export struct XrdpServerCard {
  @Prop isDark: boolean = false
  @Prop targetSection: string = ''
  @Prop running: boolean = false
  @Prop busy: boolean = false
  @Prop port: number = 3390
  @Prop message: string = ''
  onRefresh: () => void = () => {
  }
  onStart: () => void = () => {
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
    if (this.busy) {
      return SettingsText.REMOTE_SERVER_BUSY
    }
    const status = this.running ? SettingsText.REMOTE_SERVER_RUNNING : SettingsText.REMOTE_SERVER_STOPPED
    return `${status} / ${this.port}`
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
        if (this.message.length > 0) {
          Text(this.message)
            .fontSize(12)
            .fontColor(SettingsTheme.mutedText(this.isDark))
            .lineHeight(18)
            .width('100%')
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
      }
      .layoutWeight(1)

      Button(this.actionText())
        .type(ButtonType.Normal)
        .height(36)
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
        .height(36)
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
          .height(36)
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
export struct ScreenRecordingCard {
  @Prop isDark: boolean = false
  @Prop targetSection: string = ''
  @Prop granted: boolean = false
  @Prop busy: boolean = false
  onRequest: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private isTarget(): boolean {
    return this.targetSection === SettingsRemoteControlSection.SCREEN
  }

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  private statusText(): string {
    if (this.busy) {
      return SettingsText.REMOTE_SCREEN_PERMISSION_BUSY
    }
    return this.granted ? SettingsText.REMOTE_SCREEN_PERMISSION_GRANTED : SettingsText.REMOTE_SCREEN_PERMISSION_MISSING
  }

  private actionText(): string {
    if (this.busy) {
      return SettingsText.REMOTE_SCREEN_PERMISSION_BUSY
    }
    return this.granted ? SettingsText.REMOTE_SCREEN_PERMISSION_GRANTED : SettingsText.REMOTE_SCREEN_PERMISSION_ACTION
  }

  private actionDisabled(): boolean {
    return this.busy || this.granted
  }

  build() {
    Row({ space: 14 }) {
      Column() {
        Image(SettingsResources.REMOTE_SCREEN_ICON)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.CYAN))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(SettingsTheme.accentBackground(this.isDark, SettingsAccent.CYAN))

      Column({ space: 5 }) {
        Text(SettingsText.REMOTE_SCREEN_PERMISSION_TITLE)
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .width('100%')
        Text(SettingsText.REMOTE_SCREEN_PERMISSION_DESC)
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
        .height(36)
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
      color: this.isTarget() ? SettingsTheme.accentColor(this.isDark, SettingsAccent.CYAN) :
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
