import pasteboard from '@ohos.pasteboard'
import { SettingsStatusTone, SettingsTheme } from '../settings/SettingsUi'
import { HomeResources } from './HomeResources'
import { HomeText } from './HomeText'
import { HomeTheme } from './HomeTheme'

@Component
export struct HomeConnectionDetails {
  @Link host: string
  @Link port: string
  @Link username: string
  @Link password: string
  @Link rememberPassword: boolean
  @Prop selectedConnectionProfileId: string = ''
  @Prop connectionFeedbackText: string = ''
  @Prop connectionFeedbackTone: SettingsStatusTone = 'neutral'
  @Prop isDark: boolean = false
  onHostChange: (value: string) => void = (_value: string) => {
  }
  onPortChange: (value: string) => void = (_value: string) => {
  }
  onUsernameChange: (value: string) => void = (_value: string) => {
  }
  onPasswordChange: (value: string) => void = (_value: string) => {
  }
  onDeleteProfile: () => void = () => {
  }
  onRememberPasswordChange: (remember: boolean) => void = (_remember: boolean) => {
  }
  onClearPassword: () => void = () => {
  }
  onConnect: () => void = () => {
  }
  @State private hoveredAction: string = ''
  @State private pressedAction: string = ''
  @State private connectHovered: boolean = false
  @State private connectPressed: boolean = false

  private hasSelectedProfile(): boolean {
    return this.selectedConnectionProfileId.length > 0
  }

  private currentAddressText(): string {
    const hostText = this.host.trim()
    const portText = this.port.trim()
    return portText.length > 0 ? `${hostText}:${portText}` : hostText
  }

  private copyCurrentAddress(): void {
    const addressText = this.currentAddressText()
    if (addressText.length === 0) {
      return
    }
    try {
      const data = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, addressText)
      pasteboard.getSystemPasteboard().setData(data)
    } catch (_error) {
    }
  }

  private setHoveredAction(action: string, hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hoveredAction = hovered ? action : ''
    })
  }

  @Builder
  private buildHostField() {
    Row({ space: 30 }) {
      Text('Windows host')
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .textAlign(TextAlign.End)
        .width(190)
        .maxLines(1)
      TextInput({ placeholder: 'Windows host', text: $$this.host })
        .type(InputType.Normal)
        .height(48)
        .layoutWeight(1)
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .placeholderColor(SettingsTheme.mutedText(this.isDark))
        .backgroundColor(HomeTheme.fieldBackground(this.isDark))
        .borderRadius(6)
        .border({
          width: 1,
          color: HomeTheme.mutedBorderColor(this.isDark),
          style: BorderStyle.Solid
        })
        .onChange((value: string) => {
          this.host = value
          this.onHostChange(value)
        })
    }
    .width('100%')
    .height(52)
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildPortField() {
    Row({ space: 30 }) {
      Text('Port')
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .textAlign(TextAlign.End)
        .width(190)
        .maxLines(1)
      TextInput({ placeholder: 'Port', text: $$this.port })
        .type(InputType.Normal)
        .height(48)
        .layoutWeight(1)
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .placeholderColor(SettingsTheme.mutedText(this.isDark))
        .backgroundColor(HomeTheme.fieldBackground(this.isDark))
        .borderRadius(6)
        .border({
          width: 1,
          color: HomeTheme.mutedBorderColor(this.isDark),
          style: BorderStyle.Solid
        })
        .onChange((value: string) => {
          this.port = value
          this.onPortChange(value)
        })
    }
    .width('100%')
    .height(52)
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildUsernameField() {
    Row({ space: 30 }) {
      Text('Username')
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .textAlign(TextAlign.End)
        .width(190)
        .maxLines(1)
      TextInput({ placeholder: 'Username', text: $$this.username })
        .type(InputType.Normal)
        .height(48)
        .layoutWeight(1)
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .placeholderColor(SettingsTheme.mutedText(this.isDark))
        .backgroundColor(HomeTheme.fieldBackground(this.isDark))
        .borderRadius(6)
        .border({
          width: 1,
          color: HomeTheme.mutedBorderColor(this.isDark),
          style: BorderStyle.Solid
        })
        .onChange((value: string) => {
          this.username = value
          this.onUsernameChange(value)
        })
    }
    .width('100%')
    .height(52)
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildPasswordField() {
    Row({ space: 30 }) {
      Text('Password')
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .textAlign(TextAlign.End)
        .width(190)
        .maxLines(1)
      TextInput({ placeholder: 'Password', text: $$this.password })
        .type(InputType.Password)
        .height(48)
        .layoutWeight(1)
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .placeholderColor(SettingsTheme.mutedText(this.isDark))
        .backgroundColor(HomeTheme.fieldBackground(this.isDark))
        .borderRadius(6)
        .border({
          width: 1,
          color: HomeTheme.mutedBorderColor(this.isDark),
          style: BorderStyle.Solid
        })
        .onChange((value: string) => {
          this.password = value
          this.onPasswordChange(value)
        })
    }
    .width('100%')
    .height(52)
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildRememberRow() {
    Row({ space: 30 }) {
      Text(HomeText.REMEMBER_PASSWORD)
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .textAlign(TextAlign.End)
        .width(190)
        .maxLines(1)
      Row({ space: 12 }) {
        Toggle({ type: ToggleType.Switch, isOn: this.rememberPassword })
          .onChange((enabled: boolean) => {
            this.rememberPassword = enabled
            this.onRememberPasswordChange(enabled)
          })
        if (this.connectionFeedbackText.length > 0) {
          Text(this.connectionFeedbackText)
            .fontSize(12)
            .fontColor(SettingsTheme.statusText(this.isDark, this.connectionFeedbackTone))
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .layoutWeight(1)
        }
      }
      .layoutWeight(1)
      .alignItems(VerticalAlign.Center)
    }
    .width('100%')
    .height(44)
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildConnectButton() {
    Row({ space: 30 }) {
      Text('')
        .width(190)
      Button() {
        Row({ space: 10 }) {
          Image(HomeResources.LINK_ICON)
            .width(24)
            .height(24)
            .fillColor(Color.White)
            .draggable(false)
          Text('Connect')
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
            .fontColor(Color.White)
            .maxLines(1)
        }
        .justifyContent(FlexAlign.Center)
        .alignItems(VerticalAlign.Center)
      }
      .type(ButtonType.Normal)
      .height(48)
      .layoutWeight(1)
      .borderRadius(7)
      .backgroundColor(HomeTheme.primaryButtonBackground(this.isDark, this.connectHovered, this.connectPressed))
      .stateEffect(false)
      .scale({ x: this.connectPressed ? 0.99 : 1, y: this.connectPressed ? 0.99 : 1 })
      .onHover((hovered: boolean) => {
        this.connectHovered = hovered
      })
      .onTouch((event: TouchEvent) => {
        if (event.type === TouchType.Down) {
          this.connectPressed = true
          return
        }
        if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
          this.connectPressed = false
        }
      })
      .onClick(() => {
        this.connectPressed = false
        this.onConnect()
      })
    }
    .width('100%')
    .height(52)
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildActionButton(action: string, text: string, icon: Resource, tone: SettingsStatusTone,
    onPress: () => void) {
    Button() {
      Row({ space: 8 }) {
        Image(icon)
          .width(20)
          .height(20)
          .fillColor(SettingsTheme.statusText(this.isDark, tone))
          .draggable(false)
        Text(text)
          .fontSize(15)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.statusText(this.isDark, tone))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .justifyContent(FlexAlign.Center)
      .alignItems(VerticalAlign.Center)
      .width('100%')
      .padding({ left: 6, right: 6 })
    }
    .type(ButtonType.Normal)
    .height(36)
    .layoutWeight(1)
    .borderRadius(7)
    .backgroundColor(this.pressedAction === action ? SettingsTheme.statusBackground(this.isDark, tone) :
      (this.hoveredAction === action ? SettingsTheme.statusBackground(this.isDark, tone) :
        HomeTheme.fieldBackground(this.isDark)))
    .border({
      width: 1,
      color: this.hoveredAction === action || this.pressedAction === action ?
        SettingsTheme.statusText(this.isDark, tone) : SettingsTheme.statusBorder(this.isDark, tone)
    })
    .stateEffect(false)
    .scale({ x: this.pressedAction === action ? 0.98 : 1, y: this.pressedAction === action ? 0.98 : 1 })
    .onHover((hovered: boolean) => {
      this.setHoveredAction(action, hovered)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressedAction = action
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressedAction = ''
      }
    })
    .onClick(() => {
      this.pressedAction = ''
      onPress()
    })
  }

  @Builder
  private buildDeviceActions() {
    Row({ space: 12 }) {
      Text(HomeText.DEVICE_ACTIONS)
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width(170)
        .textAlign(TextAlign.End)
      this.buildActionButton('delete', HomeText.DELETE_ACTION, HomeResources.TRASH_ICON, 'danger', () => {
        this.onDeleteProfile()
      })
      this.buildActionButton('clearPassword', HomeText.CLEAR_PASSWORD_ACTION, HomeResources.LOCK_ICON, 'warning', () => {
        this.onClearPassword()
      })
      this.buildActionButton('copyAddress', HomeText.COPY_ADDRESS_ACTION, HomeResources.COPY_ICON, 'info', () => {
        this.copyCurrentAddress()
      })
    }
    .width('100%')
    .height(40)
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildConnectionFormFields() {
    Column({ space: 12 }) {
      this.buildHostField()
      this.buildPortField()
      this.buildUsernameField()
      this.buildPasswordField()
      this.buildRememberRow()
      this.buildConnectButton()
    }
    .width('100%')
    .padding({ top: 2, bottom: 6 })
  }

  build() {
    Column({ space: 12 }) {
      Text(HomeText.CONNECTION_DETAILS_TITLE)
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width('100%')
        .maxLines(1)

      this.buildConnectionFormFields()

      Divider()
        .height(1)
        .color(HomeTheme.mutedBorderColor(this.isDark))
        .margin({ top: 2, bottom: 0 })

      if (this.hasSelectedProfile()) {
        this.buildDeviceActions()
      }
    }
    .layoutWeight(1)
    .height('100%')
    .padding({ left: 34, right: 34, top: 22, bottom: 20 })
    .alignItems(HorizontalAlign.Start)
    .backgroundColor(HomeTheme.panelBackground(this.isDark))
    .borderRadius(8)
    .border({
      width: 1,
      color: HomeTheme.mutedBorderColor(this.isDark)
    })
  }
}
