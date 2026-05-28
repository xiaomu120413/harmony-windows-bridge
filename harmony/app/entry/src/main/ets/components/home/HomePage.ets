import pasteboard from '@ohos.pasteboard'
import {
  SettingsAccent,
  SettingsResources,
  SettingsStatusTone,
  SettingsTheme
} from '../settings/SettingsUi'
import { WindowsConnectionProfile } from '../../rdp/WindowsConnectionStore'

class HomeResources {
  static readonly SEARCH_ICON: Resource = $r('app.media.home_search')
  static readonly MONITOR_ICON: Resource = $r('app.media.home_monitor')
  static readonly SHIELD_ICON: Resource = $r('app.media.home_shield')
  static readonly LOCK_ICON: Resource = $r('app.media.home_lock')
  static readonly COPY_ICON: Resource = $r('app.media.home_copy')
  static readonly LINK_ICON: Resource = $r('app.media.home_link')
  static readonly TRASH_ICON: Resource = $r('app.media.home_trash')
  static readonly CHEVRON_RIGHT_ICON: Resource = $r('app.media.home_chevron_right')
}

@Component
export struct HomePage {
  @Link host: string
  @Link port: string
  @Link username: string
  @Link password: string
  @Prop connectionProfiles: WindowsConnectionProfile[] = []
  @Prop selectedConnectionProfileId: string = ''
  @Link rememberPassword: boolean
  @Prop isDark: boolean = false
  @Prop xrdpServerBusy: boolean = false
  @Prop xrdpServerState: string = 'Stopped'
  @Prop xrdpServerRunning: boolean = false
  @Prop xrdpServerPort: number = 3390
  @Prop screenRecordingPermissionGranted: boolean = false
  @Prop remoteAccessCodeGateEnabled: boolean = false
  onHostChange: (value: string) => void = (_value: string) => {
  }
  onPortChange: (value: string) => void = (_value: string) => {
  }
  onUsernameChange: (value: string) => void = (_value: string) => {
  }
  onPasswordChange: (value: string) => void = (_value: string) => {
  }
  onProfileSelect: (profileId: string) => void = (_profileId: string) => {
  }
  onNewProfile: () => void = () => {
  }
  onDeleteProfile: () => void = () => {
  }
  onRememberPasswordChange: (remember: boolean) => void = (_remember: boolean) => {
  }
  onConnect: () => void = () => {
  }
  onOpenSettings: () => void = () => {
  }
  @State private profileSearchText: string = ''
  @State private hoveredProfileId: string = ''
  @State private pressedProfileId: string = ''
  @State private hoveredAction: string = ''
  @State private pressedAction: string = ''
  @State private newDeviceHovered: boolean = false
  @State private newDevicePressed: boolean = false
  @State private settingsHovered: boolean = false
  @State private settingsPressed: boolean = false
  @State private connectHovered: boolean = false
  @State private connectPressed: boolean = false

  private panelBackground(): string {
    return SettingsTheme.cardBackground(this.isDark)
  }

  private appBackground(): string {
    return this.isDark ? '#101114' : '#F5F7FA'
  }

  private fieldBackground(): string {
    return this.isDark ? '#15171C' : '#FFFFFF'
  }

  private homeBorderColor(active: boolean = false): string {
    return active ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) : SettingsTheme.borderColor(this.isDark)
  }

  private mutedBorderColor(): string {
    return this.isDark ? '#303641' : '#D9DEE7'
  }

  private primaryButtonBackground(hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return this.isDark ? '#4E9FFF' : '#086FD8'
    }
    if (hovered) {
      return this.isDark ? '#65AEFF' : '#087AF0'
    }
    return SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE)
  }

  private neutralButtonBackground(hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return this.isDark ? '#2B3038' : '#E6EBF2'
    }
    if (hovered) {
      return this.isDark ? '#242A33' : '#EEF2F7'
    }
    return this.isDark ? '#1D2128' : '#F2F4F7'
  }

  private activePanelBackground(hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return this.isDark ? '#162B46' : '#EDF6FF'
    }
    if (hovered) {
      return this.isDark ? '#183252' : '#F2F8FF'
    }
    return this.isDark ? '#14263D' : '#F7FBFF'
  }

  private inactivePanelBackground(hovered: boolean, pressed: boolean): string {
    if (pressed) {
      return this.isDark ? '#20242B' : '#EEF2F7'
    }
    if (hovered) {
      return this.isDark ? '#1D222A' : '#FAFCFF'
    }
    return this.fieldBackground()
  }

  private headerStatusTone(): SettingsStatusTone {
    if (this.xrdpServerBusy) {
      return 'info'
    }
    if (!this.screenRecordingPermissionGranted) {
      return 'warning'
    }
    if (this.xrdpServerRunning) {
      return 'ok'
    }
    return 'neutral'
  }

  private headerStatusText(): string {
    if (this.xrdpServerBusy) {
      return '处理中'
    }
    if (!this.screenRecordingPermissionGranted) {
      return '待授权'
    }
    if (this.xrdpServerRunning) {
      return '服务正常'
    }
    return '未启动'
  }

  private serviceStatusTone(): SettingsStatusTone {
    if (this.xrdpServerBusy) {
      return 'info'
    }
    if (this.xrdpServerState === 'Failed' || this.xrdpServerState === 'Exited') {
      return 'danger'
    }
    if (this.xrdpServerRunning) {
      return 'ok'
    }
    return 'neutral'
  }

  private serviceStatusText(): string {
    if (this.xrdpServerBusy) {
      return '处理中'
    }
    const status = this.xrdpServerRunning ? '已启动' : '未启动'
    return `${status} / ${this.xrdpServerPort}`
  }

  private screenStatusTone(): SettingsStatusTone {
    return this.screenRecordingPermissionGranted ? 'ok' : 'warning'
  }

  private screenStatusText(): string {
    return this.screenRecordingPermissionGranted ? '已授权' : '未授权'
  }

  private accessStatusTone(): SettingsStatusTone {
    return this.remoteAccessCodeGateEnabled ? 'ok' : 'neutral'
  }

  private accessStatusText(): string {
    return this.remoteAccessCodeGateEnabled ? '已开启' : '默认关闭'
  }

  private hasSelectedProfile(): boolean {
    return this.selectedConnectionProfileId.length > 0
  }

  private hasConnectionProfiles(): boolean {
    return this.connectionProfiles.length > 0
  }

  private isSelectedProfile(profile: WindowsConnectionProfile): boolean {
    return profile.id === this.selectedConnectionProfileId
  }

  private profileName(profile: WindowsConnectionProfile): string {
    const slashIndex = profile.username.indexOf('\\')
    const name = slashIndex > 0 ? profile.username.substring(0, slashIndex) :
      (profile.name.length > 0 ? profile.name : profile.host)
    return name.length > 20 ? `${name.substring(0, 20)}...` : name
  }

  private profileAddress(profile: WindowsConnectionProfile): string {
    return `${profile.host}:${profile.port}`
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

  private normalizedSearchText(): string {
    return this.profileSearchText.trim().toLowerCase()
  }

  private profileVisible(profile: WindowsConnectionProfile): boolean {
    const search = this.normalizedSearchText()
    if (search.length === 0) {
      return true
    }
    const haystack = `${profile.name} ${profile.host} ${profile.port} ${profile.username}`.toLowerCase()
    return haystack.indexOf(search) >= 0
  }

  private visibleProfileCount(): number {
    let count = 0
    for (const profile of this.connectionProfiles) {
      if (this.profileVisible(profile)) {
        count++
      }
    }
    return count
  }

  private setHoveredProfile(profileId: string, hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hoveredProfileId = hovered ? profileId : ''
    })
  }

  private setHoveredAction(action: string, hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hoveredAction = hovered ? action : ''
    })
  }

  @Builder
  private buildHeaderStatus() {
    Row({ space: 10 }) {
      Circle()
        .width(9)
        .height(9)
        .fill(SettingsTheme.statusText(this.isDark, this.headerStatusTone()))
      Text(this.headerStatusText())
        .fontSize(15)
        .fontWeight(FontWeight.Medium)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
    }
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private buildSettingsButton() {
    Row({ space: 8 }) {
      Image(SettingsResources.SETTINGS_ICON)
        .width(22)
        .height(22)
        .fillColor(SettingsTheme.primaryText(this.isDark))
        .draggable(false)
      Text('设置')
        .fontSize(15)
        .fontWeight(FontWeight.Medium)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
    }
    .height(44)
    .padding({ left: 14, right: 14 })
    .alignItems(VerticalAlign.Center)
    .borderRadius(8)
    .backgroundColor(this.neutralButtonBackground(this.settingsHovered, this.settingsPressed))
    .scale({ x: this.settingsPressed ? 0.98 : 1, y: this.settingsPressed ? 0.98 : 1 })
    .onHover((hovered: boolean) => {
      this.settingsHovered = hovered
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.settingsPressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.settingsPressed = false
      }
    })
    .onClick(() => {
      this.settingsPressed = false
      this.onOpenSettings()
    })
  }

  @Builder
  private buildHeader() {
    Row() {
      Row()
        .width(380)

      Text('连接 Windows 主机')
        .fontSize(22)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
        .textAlign(TextAlign.Center)
        .layoutWeight(1)

      Row({ space: 18 }) {
        this.buildHeaderStatus()
        Divider()
          .vertical(true)
          .height(30)
          .color(this.homeBorderColor())
        this.buildSettingsButton()
      }
      .width(380)
      .justifyContent(FlexAlign.End)
      .alignItems(VerticalAlign.Center)
    }
    .width('100%')
    .height(84)
    .padding({ left: 32, right: 30 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(this.panelBackground())
    .border({
      width: { bottom: 1 },
      color: this.homeBorderColor()
    })
  }

  @Builder
  private buildNewDeviceButton() {
    Row({ space: 8 }) {
      Text('+')
        .fontSize(22)
        .fontColor(Color.White)
        .fontWeight(FontWeight.Regular)
        .height(26)
      Text('新建设备')
        .fontSize(15)
        .fontColor(Color.White)
        .fontWeight(FontWeight.Bold)
        .maxLines(1)
    }
    .width(150)
    .height(48)
    .justifyContent(FlexAlign.Center)
    .alignItems(VerticalAlign.Center)
    .borderRadius(8)
    .backgroundColor(this.primaryButtonBackground(this.newDeviceHovered, this.newDevicePressed))
    .scale({ x: this.newDevicePressed ? 0.98 : 1, y: this.newDevicePressed ? 0.98 : 1 })
    .onHover((hovered: boolean) => {
      this.newDeviceHovered = hovered
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.newDevicePressed = true
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.newDevicePressed = false
      }
    })
    .onClick(() => {
      this.newDevicePressed = false
      this.onNewProfile()
    })
  }

  @Builder
  private buildSearchBox() {
    Row({ space: 12 }) {
      Image(HomeResources.SEARCH_ICON)
        .width(22)
        .height(22)
        .fillColor(SettingsTheme.mutedText(this.isDark))
        .draggable(false)
      TextInput({ placeholder: '搜索设备名称或地址', text: this.profileSearchText })
        .height(48)
        .layoutWeight(1)
        .fontSize(15)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .placeholderColor(SettingsTheme.mutedText(this.isDark))
        .backgroundColor('rgba(0,0,0,0)')
        .borderRadius(0)
        .onChange((value: string) => {
          this.profileSearchText = value
        })
    }
    .width('100%')
    .height(58)
    .padding({ left: 16, right: 10 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(this.fieldBackground())
    .borderRadius(8)
    .border({
      width: 1,
      color: this.mutedBorderColor(),
      style: BorderStyle.Solid
    })
  }

  @Builder
  private buildDeviceTag(text: string) {
    Text(text)
      .fontSize(12)
      .fontWeight(FontWeight.Medium)
      .fontColor(SettingsTheme.statusText(this.isDark, 'ok'))
      .height(24)
      .padding({ left: 9, right: 9 })
      .backgroundColor(SettingsTheme.statusBackground(this.isDark, 'ok'))
      .borderRadius(7)
      .maxLines(1)
  }

  @Builder
  private buildDeviceIcon(selected: boolean) {
    Column() {
      Image(HomeResources.MONITOR_ICON)
        .width(31)
        .height(31)
        .fillColor(Color.White)
        .draggable(false)
    }
    .width(60)
    .height(60)
    .borderRadius(30)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .backgroundColor(selected ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) : '#8B9099')
  }

  @Builder
  private buildDeviceCard(profile: WindowsConnectionProfile) {
    Row({ space: 18 }) {
      this.buildDeviceIcon(this.isSelectedProfile(profile))

      Column({ space: 8 }) {
        Text(this.profileName(profile))
          .fontSize(19)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
        Text(this.profileAddress(profile))
          .fontSize(15)
          .fontColor(SettingsTheme.secondaryText(this.isDark))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
        Row({ space: 8 }) {
          if (this.isSelectedProfile(profile)) {
            this.buildDeviceTag('最近连接')
          }
          if (profile.rememberPassword) {
            this.buildDeviceTag('已保存密码')
          }
        }
        .height(24)
        .alignItems(VerticalAlign.Center)
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)

      Image(HomeResources.CHEVRON_RIGHT_ICON)
        .width(26)
        .height(26)
        .fillColor(this.isSelectedProfile(profile) ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) :
          SettingsTheme.mutedText(this.isDark))
        .draggable(false)
    }
    .width('100%')
    .height(128)
    .padding({ left: 22, right: 20 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(this.isSelectedProfile(profile) ?
      this.activePanelBackground(this.hoveredProfileId === profile.id, this.pressedProfileId === profile.id) :
      this.inactivePanelBackground(this.hoveredProfileId === profile.id, this.pressedProfileId === profile.id))
    .borderRadius(8)
    .border({
      width: this.isSelectedProfile(profile) ? 2 : 1,
      color: this.isSelectedProfile(profile) ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) :
        this.mutedBorderColor()
    })
    .shadow({
      radius: this.hoveredProfileId === profile.id ? 8 : 0,
      color: this.isDark ? '#26000000' : '#10111827',
      offsetX: 0,
      offsetY: this.hoveredProfileId === profile.id ? 4 : 0
    })
    .scale({
      x: this.pressedProfileId === profile.id ? 0.99 : 1,
      y: this.pressedProfileId === profile.id ? 0.99 : 1
    })
    .onHover((hovered: boolean) => {
      this.setHoveredProfile(profile.id, hovered)
    })
    .onTouch((event: TouchEvent) => {
      if (event.type === TouchType.Down) {
        this.pressedProfileId = profile.id
        return
      }
      if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.pressedProfileId = ''
      }
    })
    .onClick(() => {
      this.pressedProfileId = ''
      this.onProfileSelect(profile.id)
    })
  }

  @Builder
  private buildDeviceEmptyState(title: string, body: string) {
    Column({ space: 8 }) {
      Text(title)
        .fontSize(16)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
      Text(body)
        .fontSize(13)
        .fontColor(SettingsTheme.mutedText(this.isDark))
        .maxLines(1)
    }
    .width('100%')
    .height(128)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .borderRadius(8)
    .border({
      width: 1,
      color: this.mutedBorderColor()
    })
  }

  @Builder
  private buildDeviceList() {
    Column({ space: 20 }) {
      Row() {
        Text('设备列表')
          .fontSize(24)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .maxLines(1)
          .layoutWeight(1)

        this.buildNewDeviceButton()
      }
      .width('100%')
      .alignItems(VerticalAlign.Center)

      this.buildSearchBox()

      if (!this.hasConnectionProfiles()) {
        this.buildDeviceEmptyState('还没有保存的设备', '连接成功后会自动出现在这里')
      } else if (this.visibleProfileCount() === 0) {
        this.buildDeviceEmptyState('没有匹配设备', '换个设备名称或地址再试')
      } else {
        Column({ space: 18 }) {
          ForEach(this.connectionProfiles, (profile: WindowsConnectionProfile) => {
            if (this.profileVisible(profile)) {
              this.buildDeviceCard(profile)
            }
          }, (profile: WindowsConnectionProfile) => profile.id)
        }
        .width('100%')
      }
    }
    .width('32%')
    .height('100%')
    .padding({ left: 22, right: 22, top: 24, bottom: 24 })
    .alignItems(HorizontalAlign.Start)
    .backgroundColor(this.panelBackground())
    .borderRadius(8)
    .border({
      width: 1,
      color: this.mutedBorderColor()
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
        .backgroundColor(this.fieldBackground())
        .borderRadius(6)
        .border({
          width: 1,
          color: this.mutedBorderColor(),
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
        .backgroundColor(this.fieldBackground())
        .borderRadius(6)
        .border({
          width: 1,
          color: this.mutedBorderColor(),
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
        .backgroundColor(this.fieldBackground())
        .borderRadius(6)
        .border({
          width: 1,
          color: this.mutedBorderColor(),
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
        .backgroundColor(this.fieldBackground())
        .borderRadius(6)
        .border({
          width: 1,
          color: this.mutedBorderColor(),
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
      Text('记住密码')
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .textAlign(TextAlign.End)
        .width(190)
        .maxLines(1)
      Toggle({ type: ToggleType.Switch, isOn: this.rememberPassword })
        .onChange((enabled: boolean) => {
          this.rememberPassword = enabled
          this.onRememberPasswordChange(enabled)
        })
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
      .height(52)
      .layoutWeight(1)
      .borderRadius(7)
      .backgroundColor(this.primaryButtonBackground(this.connectHovered, this.connectPressed))
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
    .height(56)
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
      }
      .justifyContent(FlexAlign.Center)
      .alignItems(VerticalAlign.Center)
    }
    .type(ButtonType.Normal)
    .height(40)
    .width(144)
    .borderRadius(7)
    .backgroundColor(this.pressedAction === action ? SettingsTheme.statusBackground(this.isDark, tone) :
      (this.hoveredAction === action ? SettingsTheme.statusBackground(this.isDark, tone) : this.fieldBackground()))
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
    Row({ space: 24 }) {
      Text('设备操作')
        .fontSize(16)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width(190)
        .textAlign(TextAlign.End)
      this.buildActionButton('delete', '删除', HomeResources.TRASH_ICON, 'danger', () => {
        this.onDeleteProfile()
      })
      this.buildActionButton('clearPassword', '清除密码', HomeResources.LOCK_ICON, 'warning', () => {
        this.password = ''
        this.rememberPassword = false
        this.onPasswordChange('')
        this.onRememberPasswordChange(false)
      })
      this.buildActionButton('copyAddress', '复制地址', HomeResources.COPY_ICON, 'info', () => {
        this.copyCurrentAddress()
      })
    }
    .width('100%')
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

  @Builder
  private buildConnectionDetails() {
    Column({ space: 14 }) {
      Text('连接详情')
        .fontSize(24)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width('100%')
        .maxLines(1)

      this.buildConnectionFormFields()

      Divider()
        .height(1)
        .color(this.mutedBorderColor())
        .margin({ top: 2, bottom: 0 })

      if (this.hasSelectedProfile()) {
        this.buildDeviceActions()
      }
    }
    .layoutWeight(1)
    .height('100%')
    .padding({ left: 34, right: 34, top: 22, bottom: 20 })
    .alignItems(HorizontalAlign.Start)
    .backgroundColor(this.panelBackground())
    .borderRadius(8)
    .border({
      width: 1,
      color: this.mutedBorderColor()
    })
  }

  @Builder
  private buildStatusIcon(icon: Resource, tone: SettingsStatusTone) {
    Column() {
      Image(icon)
        .width(24)
        .height(24)
        .fillColor(SettingsTheme.statusText(this.isDark, tone))
        .draggable(false)
    }
    .width(36)
    .height(36)
    .borderRadius(8)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .backgroundColor(SettingsTheme.statusBackground(this.isDark, tone))
  }

  @Builder
  private buildFooterStatusCard(label: string, value: string, icon: Resource, tone: SettingsStatusTone) {
    Row({ space: 12 }) {
      this.buildStatusIcon(icon, tone)
      Text(label)
        .fontSize(16)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
      Text(value)
        .fontSize(15)
        .fontColor(SettingsTheme.secondaryText(this.isDark))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .layoutWeight(1)
      Circle()
        .width(9)
        .height(9)
        .fill(SettingsTheme.statusText(this.isDark, tone))
    }
    .height(60)
    .layoutWeight(1)
    .padding({ left: 16, right: 16 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(this.panelBackground())
    .borderRadius(8)
    .border({
      width: 1,
      color: this.mutedBorderColor()
    })
  }

  @Builder
  private buildFooter() {
    Row({ space: 22 }) {
      this.buildFooterStatusCard('被控服务', this.serviceStatusText(), HomeResources.SHIELD_ICON, this.serviceStatusTone())
      this.buildFooterStatusCard('录屏', this.screenStatusText(), HomeResources.MONITOR_ICON, this.screenStatusTone())
      this.buildFooterStatusCard('验证码', this.accessStatusText(), HomeResources.SHIELD_ICON, this.accessStatusTone())
      this.buildFooterStatusCard('共享目录', '已配置', HomeResources.LOCK_ICON, 'ok')
    }
    .width('100%')
    .height(82)
    .padding({ left: 30, right: 30, top: 10, bottom: 12 })
    .backgroundColor(this.appBackground())
    .border({
      width: { top: 1 },
      color: this.homeBorderColor()
    })
  }

  @Builder
  private buildMain() {
    Row({ space: 18 }) {
      this.buildDeviceList()
      this.buildConnectionDetails()
    }
    .width('100%')
    .layoutWeight(1)
    .padding({ left: 30, right: 30, top: 22, bottom: 22 })
    .backgroundColor(this.appBackground())
  }

  build() {
    Column() {
      this.buildHeader()
      this.buildMain()
      this.buildFooter()
    }
    .width('100%')
    .height('100%')
    .backgroundColor(this.appBackground())
  }
}
