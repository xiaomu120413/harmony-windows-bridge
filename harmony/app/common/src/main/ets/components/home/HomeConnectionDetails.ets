import { LayoutMode } from '../../adaptive/WindowLayoutPolicy'
import { SettingsStatusTone, SettingsTheme } from '../settings/SettingsUi'
import { HomeResources } from './HomeResources'
import { HomeText } from './HomeText'
import { HomeTheme } from './HomeTheme'

@Component
export struct HomeConnectionDetails {
  @Prop layoutMode: LayoutMode = LayoutMode.COMPACT
  @Link host: string
  @Link port: string
  @Link username: string
  @Link password: string
  @Link rememberPassword: boolean
  @Prop passwordLoading: boolean = false
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
  @State private formBreakpoint: string = 'xs'

  private hasSelectedProfile(): boolean {
    return this.selectedConnectionProfileId.length > 0
  }

  private setHoveredAction(action: string, hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hoveredAction = hovered ? action : ''
    })
  }

  @Builder
  private buildFieldLabel(text: string) {
    Text(text)
      .fontSize(16)
      .fontColor(SettingsTheme.primaryText(this.isDark))
      .textAlign(this.formBreakpoint === 'xs' ? TextAlign.Start : TextAlign.End)
      .width('100%')
      .constraintSize({ minHeight: 48 })
      .maxLines(2)
  }

  @Builder
  private buildHostInput() {
    TextInput({ placeholder: 'Windows host', text: $$this.host })
      .type(InputType.Normal)
      .constraintSize({ minHeight: 48 })
      .width('100%')
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

  @Builder
  private buildPortInput() {
    TextInput({ placeholder: 'Port', text: $$this.port })
      .type(InputType.Normal)
      .constraintSize({ minHeight: 48 })
      .width('100%')
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

  @Builder
  private buildUsernameInput() {
    TextInput({ placeholder: HomeText.WINDOWS_USERNAME_PLACEHOLDER, text: $$this.username })
      .type(InputType.Normal)
      .constraintSize({ minHeight: 48 })
      .width('100%')
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

  @Builder
  private buildPasswordInput() {
    TextInput({ placeholder: 'Password', text: $$this.password })
      .type(InputType.Password)
      .constraintSize({ minHeight: 48 })
      .width('100%')
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
      .enabled(!this.passwordLoading)
      .onChange((value: string) => {
        this.password = value
        this.onPasswordChange(value)
      })
  }

  @Builder
  private buildRememberControl() {
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
          .maxLines(3)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .layoutWeight(1)
      }
    }
    .width('100%')
    .constraintSize({ minHeight: 48 })
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildConnectButton() {
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
          .maxLines(2)
      }
      .justifyContent(FlexAlign.Center)
      .alignItems(VerticalAlign.Center)
    }
    .type(ButtonType.Normal)
    .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
    .width('100%')
    .borderRadius(7)
    .backgroundColor(HomeTheme.primaryButtonBackground(this.isDark, this.connectHovered, this.connectPressed))
    .stateEffect(false)
    .enabled(!this.passwordLoading)
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
    .constraintSize({ minHeight: SettingsTheme.SECONDARY_ACTION_HEIGHT })
    .width('100%')
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
    GridRow({
      columns: 12,
      gutter: { x: 12, y: 8 },
      breakpoints: { value: ['600vp'], reference: BreakpointsReference.ComponentSize }
    }) {
      GridCol({ span: { xs: 12, sm: 3 } }) {
        this.buildFieldLabel(HomeText.DEVICE_ACTIONS)
      }
      GridCol({ span: { xs: 12, sm: 9 } }) {
        GridRow({ columns: 12, gutter: { x: 12, y: 8 } }) {
          GridCol({ span: 6 }) {
            this.buildActionButton('delete', HomeText.DELETE_ACTION, HomeResources.TRASH_ICON, 'danger', () => {
              this.onDeleteProfile()
            })
          }
          GridCol({ span: 6 }) {
            this.buildActionButton('clearPassword', HomeText.CLEAR_PASSWORD_ACTION,
              HomeResources.LOCK_ICON, 'warning', () => {
                this.onClearPassword()
              })
          }
        }
        .width('100%')
      }
    }
    .width('100%')
    .onBreakpointChange((breakpoint: string) => {
      this.formBreakpoint = breakpoint
    })
  }

  @Builder
  private buildConnectionFormFields() {
    GridRow({
      columns: 12,
      gutter: { x: 24, y: 12 },
      breakpoints: { value: ['600vp'], reference: BreakpointsReference.ComponentSize }
    }) {
      GridCol({ span: { xs: 12, sm: 3 } }) {
        this.buildFieldLabel('Windows host')
      }
      GridCol({ span: { xs: 12, sm: 9 } }) {
        this.buildHostInput()
      }
      GridCol({ span: { xs: 12, sm: 3 } }) {
        this.buildFieldLabel('Port')
      }
      GridCol({ span: { xs: 12, sm: 9 } }) {
        this.buildPortInput()
      }
      GridCol({ span: { xs: 12, sm: 3 } }) {
        this.buildFieldLabel(HomeText.WINDOWS_USERNAME_LABEL)
      }
      GridCol({ span: { xs: 12, sm: 9 } }) {
        this.buildUsernameInput()
      }
      GridCol({ span: { xs: 12, sm: 3 } }) {
        this.buildFieldLabel('Password')
      }
      GridCol({ span: { xs: 12, sm: 9 } }) {
        this.buildPasswordInput()
      }
      GridCol({ span: { xs: 12, sm: 3 } }) {
        this.buildFieldLabel(HomeText.REMEMBER_PASSWORD)
      }
      GridCol({ span: { xs: 12, sm: 9 } }) {
        this.buildRememberControl()
      }
      GridCol({ span: { xs: 12, sm: 9 }, offset: { xs: 0, sm: 3 } }) {
        this.buildConnectButton()
      }
    }
    .width('100%')
    .padding({ top: 2, bottom: 6 })
    .onBreakpointChange((breakpoint: string) => {
      this.formBreakpoint = breakpoint
    })
  }

  build() {
    Column({ space: 12 }) {
      Text(HomeText.CONNECTION_DETAILS_TITLE)
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width('100%')
        .maxLines(2)

      this.buildConnectionFormFields()

      Divider()
        .height(1)
        .color(HomeTheme.mutedBorderColor(this.isDark))
        .margin({ top: 2, bottom: 0 })

      if (this.hasSelectedProfile()) {
        this.buildDeviceActions()
      }
    }
    .width('100%')
    .padding(this.layoutMode === LayoutMode.EXPANDED ?
      { left: 34, right: 34, top: 22, bottom: 20 } : { left: 20, right: 20, top: 16, bottom: 20 })
    .alignItems(HorizontalAlign.Start)
    .backgroundColor(HomeTheme.panelBackground(this.isDark))
    .borderRadius(8)
    .border({
      width: 1,
      color: HomeTheme.mutedBorderColor(this.isDark)
    })
  }
}
