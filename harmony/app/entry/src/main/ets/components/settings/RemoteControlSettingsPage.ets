import {
  SettingsAccent,
  SettingsPageHeader,
  SettingsResources,
  SettingsSectionTitle,
  SettingsTheme
} from './SettingsUi'
import { XrdpServerStatus } from '../../rdp/XrdpServerController'
import { SettingsText } from './SettingsConstants'

@Component
export struct RemoteControlSettingsPage {
  onBack: () => void = () => {
  }
  onRemoteAccessCodeGateChange: (enabled: boolean) => string = (_enabled: boolean) => {
    return ''
  }
  onRemoteAccessCodeRegenerate: () => string = () => {
    return ''
  }
  onRequestScreenRecordingPermission: () => Promise<boolean> = () => {
    return Promise.resolve(false)
  }
  onRefreshScreenRecordingPermission: () => boolean = () => {
    return false
  }
  onRefreshXrdpServerStatus: () => XrdpServerStatus = () => {
    return {
      running: false,
      state: 'Stopped',
      port: 3390,
      message: ''
    }
  }
  onStartXrdpServer: () => Promise<XrdpServerStatus> = () => {
    const status: XrdpServerStatus = {
      running: false,
      state: 'Stopped',
      port: 3390,
      message: ''
    }
    return Promise.resolve(status)
  }
  onOpenRemoteFilesDirectory: () => void = () => {
  }
  @Prop isDark: boolean = false
  @Prop remoteAccessCode: string = '000000'
  @Prop remoteAccessCodeGateEnabled: boolean = false
  @Prop screenRecordingPermissionGranted: boolean = false
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerPort: number = 3390
  @Prop xrdpServerMessage: string = ''
  @Prop xrdpServerBusy: boolean = false
  @State private localAccessCode: string = '000000'
  @State private localGateEnabled: boolean = false
  @State private localScreenRecordingGranted: boolean = false
  @State private localXrdpServerRunning: boolean = false
  @State private localXrdpServerState: string = 'Stopped'
  @State private localXrdpServerPort: number = 3390
  @State private localXrdpServerMessage: string = ''
  @State private localXrdpServerBusy: boolean = false
  @State private serverCardHovered: boolean = false
  @State private serverCardPressed: boolean = false
  @State private accessCardHovered: boolean = false
  @State private accessCardPressed: boolean = false
  @State private screenCardHovered: boolean = false
  @State private screenCardPressed: boolean = false
  @State private filesCardHovered: boolean = false
  @State private filesCardPressed: boolean = false
  @State private screenPermissionBusy: boolean = false

  aboutToAppear(): void {
    this.localAccessCode = this.remoteAccessCode
    this.localGateEnabled = this.remoteAccessCodeGateEnabled
    this.localScreenRecordingGranted = this.screenRecordingPermissionGranted
    this.localXrdpServerRunning = this.xrdpServerRunning
    this.localXrdpServerState = this.xrdpServerState
    this.localXrdpServerPort = this.xrdpServerPort
    this.localXrdpServerMessage = this.xrdpServerMessage
    this.localXrdpServerBusy = this.xrdpServerBusy
    this.refreshScreenRecordingState()
    this.refreshXrdpServerStatus()
  }

  private setServerCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.serverCardHovered = hovered
    })
  }

  private setAccessCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.accessCardHovered = hovered
    })
  }

  private setScreenCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.screenCardHovered = hovered
    })
  }

  private setFilesCardHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.filesCardHovered = hovered
    })
  }

  private setGateEnabled(enabled: boolean): void {
    this.localGateEnabled = enabled
    const nextCode = this.onRemoteAccessCodeGateChange(enabled)
    if (nextCode.length > 0) {
      this.localAccessCode = nextCode
    }
  }

  private regenerateAccessCode(): void {
    const nextCode = this.onRemoteAccessCodeRegenerate()
    if (nextCode.length > 0) {
      this.localAccessCode = nextCode
    }
  }

  private applyXrdpServerStatus(status: XrdpServerStatus): void {
    this.localXrdpServerRunning = status.running
    this.localXrdpServerState = status.state.length > 0 ? status.state : 'Stopped'
    this.localXrdpServerPort = status.port > 0 ? status.port : 3390
    this.localXrdpServerMessage = status.message
  }

  private refreshXrdpServerStatus(): void {
    this.applyXrdpServerStatus(this.onRefreshXrdpServerStatus())
  }

  private startXrdpServer(): void {
    if (this.localXrdpServerBusy) {
      return
    }
    this.localXrdpServerBusy = true
    this.onStartXrdpServer()
      .then((status: XrdpServerStatus) => {
        this.applyXrdpServerStatus(status)
        this.localScreenRecordingGranted = this.localScreenRecordingGranted ||
          this.onRefreshScreenRecordingPermission()
      })
      .catch((_error: Error) => {
        this.refreshXrdpServerStatus()
      })
      .finally(() => {
        this.localXrdpServerBusy = false
      })
  }

  private refreshScreenRecordingState(): void {
    this.localScreenRecordingGranted = this.onRefreshScreenRecordingPermission()
  }

  private requestScreenRecordingPermission(): void {
    if (this.screenPermissionBusy) {
      return
    }
    this.screenPermissionBusy = true
    this.onRequestScreenRecordingPermission()
      .then((granted: boolean) => {
        this.localScreenRecordingGranted = granted || this.onRefreshScreenRecordingPermission()
        this.refreshXrdpServerStatus()
      })
      .catch((_error: Error) => {
        this.localScreenRecordingGranted = this.onRefreshScreenRecordingPermission()
      })
      .finally(() => {
        this.screenPermissionBusy = false
      })
  }

  private gateStatusText(): string {
    return this.localGateEnabled ? SettingsText.REMOTE_ACCESS_GATE_ON : SettingsText.REMOTE_ACCESS_GATE_OFF
  }

  private accessCodeText(): string {
    return this.localGateEnabled ? this.localAccessCode : SettingsText.REMOTE_ACCESS_CODE_DISABLED
  }

  private screenStatusText(): string {
    if (this.screenPermissionBusy) {
      return SettingsText.REMOTE_SCREEN_PERMISSION_BUSY
    }
    return this.localScreenRecordingGranted ? SettingsText.REMOTE_SCREEN_PERMISSION_GRANTED :
      SettingsText.REMOTE_SCREEN_PERMISSION_MISSING
  }

  private xrdpServerStatusText(): string {
    if (this.localXrdpServerBusy) {
      return SettingsText.REMOTE_SERVER_BUSY
    }
    const status = this.localXrdpServerRunning ? SettingsText.REMOTE_SERVER_RUNNING :
      SettingsText.REMOTE_SERVER_STOPPED
    return `${status} · 端口 ${this.localXrdpServerPort}`
  }

  private xrdpServerActionText(): string {
    if (this.localXrdpServerBusy) {
      return SettingsText.REMOTE_SERVER_BUSY
    }
    return this.localXrdpServerRunning ? SettingsText.REMOTE_SERVER_REFRESH_ACTION :
      SettingsText.REMOTE_SERVER_START_ACTION
  }

  @Builder
  private xrdpServerCard() {
    Row({ space: 14 }) {
      Column() {
        Image(SettingsResources.DASHBOARD_ICON)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(SettingsTheme.accentColor(this.isDark, this.localXrdpServerRunning ? SettingsAccent.GREEN :
            SettingsAccent.AMBER))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(SettingsTheme.accentBackground(this.isDark, this.localXrdpServerRunning ?
        SettingsAccent.GREEN : SettingsAccent.AMBER))

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
        Text(this.xrdpServerStatusText())
          .fontSize(12)
          .fontColor(SettingsTheme.accentColor(this.isDark, this.localXrdpServerRunning ? SettingsAccent.GREEN :
            SettingsAccent.AMBER))
          .width('100%')
        if (this.localXrdpServerMessage.length > 0) {
          Text(this.localXrdpServerMessage)
            .fontSize(12)
            .fontColor(SettingsTheme.mutedText(this.isDark))
            .lineHeight(18)
            .width('100%')
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
      }
      .layoutWeight(1)

      Button(this.xrdpServerActionText())
        .type(ButtonType.Normal)
        .height(36)
        .borderRadius(SettingsTheme.BUTTON_RADIUS)
        .backgroundColor(SettingsTheme.subtleButton(this.isDark, false, this.localXrdpServerBusy))
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .fontSize(12)
        .stateEffect(false)
        .onClick(() => {
          if (this.localXrdpServerRunning) {
            this.refreshXrdpServerStatus()
            return
          }
          this.startXrdpServer()
        })
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding(16)
    .backgroundColor(this.serverCardHovered || this.serverCardPressed ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.serverCardHovered))
    .translate({ y: this.serverCardHovered ? -5 : 0 })
    .scale({
      x: this.serverCardPressed ? 0.99 : this.serverCardHovered ? 1.012 : 1,
      y: this.serverCardPressed ? 0.99 : this.serverCardHovered ? 1.012 : 1
    })
    .margin({ bottom: 12 })
    .onHover((isHover: boolean) => {
      this.setServerCardHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.serverCardPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.serverCardPressed = false
      }
    })
  }

  @Builder
  private remoteFilesCard() {
    Row({ space: 14 }) {
      Column() {
        Image(SettingsResources.HELP_ICON)
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
    .backgroundColor(this.filesCardHovered || this.filesCardPressed ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.filesCardHovered))
    .translate({ y: this.filesCardHovered ? -5 : 0 })
    .scale({
      x: this.filesCardPressed ? 0.99 : this.filesCardHovered ? 1.012 : 1,
      y: this.filesCardPressed ? 0.99 : this.filesCardHovered ? 1.012 : 1
    })
    .margin({ bottom: 12 })
    .onHover((isHover: boolean) => {
      this.setFilesCardHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.filesCardPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.filesCardPressed = false
      }
    })
  }

  @Builder
  private remoteAccessCard() {
    Column({ space: 14 }) {
      Row({ space: 14 }) {
        Column() {
          Image(SettingsResources.DASHBOARD_ICON)
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

        Toggle({ type: ToggleType.Switch, isOn: this.localGateEnabled })
          .onChange((enabled: boolean) => {
            this.setGateEnabled(enabled)
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
          .backgroundColor(SettingsTheme.subtleButton(this.isDark, false, false))
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .fontSize(12)
          .stateEffect(false)
          .onClick(() => {
            this.regenerateAccessCode()
          })
      }
      .alignItems(VerticalAlign.Center)
      .width('100%')

      Text(this.gateStatusText())
        .fontSize(12)
        .fontColor(SettingsTheme.accentColor(this.isDark, this.localGateEnabled ? SettingsAccent.GREEN :
          SettingsAccent.AMBER))
        .width('100%')
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .padding(16)
    .backgroundColor(this.accessCardHovered || this.accessCardPressed ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.accessCardHovered))
    .translate({ y: this.accessCardHovered ? -5 : 0 })
    .scale({
      x: this.accessCardPressed ? 0.99 : this.accessCardHovered ? 1.012 : 1,
      y: this.accessCardPressed ? 0.99 : this.accessCardHovered ? 1.012 : 1
    })
    .margin({ bottom: 12 })
    .onHover((isHover: boolean) => {
      this.setAccessCardHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.accessCardPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.accessCardPressed = false
      }
    })
  }

  @Builder
  private screenRecordingCard() {
    Row({ space: 14 }) {
      Column() {
        Image(SettingsResources.SETTINGS_ICON)
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
        Text(this.screenStatusText())
          .fontSize(12)
          .fontColor(SettingsTheme.accentColor(this.isDark, this.localScreenRecordingGranted ? SettingsAccent.GREEN :
            SettingsAccent.AMBER))
          .width('100%')
      }
      .layoutWeight(1)

      Button(this.screenPermissionBusy ? SettingsText.REMOTE_SCREEN_PERMISSION_BUSY :
        (this.localScreenRecordingGranted ? SettingsText.REMOTE_SCREEN_PERMISSION_GRANTED :
          SettingsText.REMOTE_SCREEN_PERMISSION_ACTION))
        .type(ButtonType.Normal)
        .height(36)
        .borderRadius(SettingsTheme.BUTTON_RADIUS)
        .backgroundColor(SettingsTheme.subtleButton(this.isDark, false, this.screenPermissionBusy))
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .fontSize(12)
        .stateEffect(false)
        .onClick(() => {
          this.requestScreenRecordingPermission()
        })
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding(16)
    .backgroundColor(this.screenCardHovered || this.screenCardPressed ?
      SettingsTheme.cardHoverBackground(this.isDark) : SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.screenCardHovered))
    .translate({ y: this.screenCardHovered ? -5 : 0 })
    .scale({
      x: this.screenCardPressed ? 0.99 : this.screenCardHovered ? 1.012 : 1,
      y: this.screenCardPressed ? 0.99 : this.screenCardHovered ? 1.012 : 1
    })
    .margin({ bottom: 10 })
    .onHover((isHover: boolean) => {
      this.setScreenCardHovered(isHover)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.screenCardPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.screenCardPressed = false
      }
    })
  }

  build() {
    Column() {
      SettingsPageHeader({
        title: SettingsText.REMOTE_CONTROL_TITLE,
        subtitle: SettingsText.REMOTE_CONTROL_SUBTITLE,
        isDark: this.isDark,
        onBack: this.onBack
      })

      Scroll() {
        Column() {
          SettingsSectionTitle({
            title: SettingsText.REMOTE_SERVER_SECTION,
            subtitle: SettingsText.REMOTE_SERVER_SECTION_DESC,
            isDark: this.isDark
          })
          this.xrdpServerCard()

          SettingsSectionTitle({
            title: SettingsText.REMOTE_ACCESS_SECTION,
            subtitle: SettingsText.REMOTE_ACCESS_SECTION_DESC,
            isDark: this.isDark
          })
          this.remoteAccessCard()

          SettingsSectionTitle({
            title: SettingsText.REMOTE_SCREEN_SECTION,
            subtitle: SettingsText.REMOTE_SCREEN_SECTION_DESC,
            isDark: this.isDark
          })
          this.screenRecordingCard()

          SettingsSectionTitle({
            title: SettingsText.REMOTE_FILES_SECTION,
            subtitle: SettingsText.REMOTE_FILES_SECTION_DESC,
            isDark: this.isDark
          })
          this.remoteFilesCard()
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
