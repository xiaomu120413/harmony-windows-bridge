import { SettingsAccent, SettingsTheme } from '../settings/SettingsUi'
import { WindowsConnectionProfile } from '../../rdp/WindowsConnectionStore'
import { HomeResources } from './HomeResources'
import { HomeText } from './HomeText'
import { HomeTheme } from './HomeTheme'

@Component
export struct HomeDeviceList {
  @Prop connectionProfiles: WindowsConnectionProfile[] = []
  @Prop selectedConnectionProfileId: string = ''
  @Prop isDark: boolean = false
  onProfileSelect: (profileId: string) => void = (_profileId: string) => {
  }
  onNewProfile: () => void = () => {
  }
  @State private profileSearchText: string = ''
  @State private hoveredProfileId: string = ''
  @State private pressedProfileId: string = ''
  @State private newDeviceHovered: boolean = false
  @State private newDevicePressed: boolean = false

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

  @Builder
  private buildNewDeviceButton() {
    Row({ space: 8 }) {
      Image(HomeResources.ADD_DEVICE_ICON)
        .width(20)
        .height(20)
        .fillColor(Color.White)
        .draggable(false)
      Text(HomeText.NEW_DEVICE_ACTION)
        .fontSize(14)
        .fontColor(Color.White)
        .fontWeight(FontWeight.Bold)
        .maxLines(1)
    }
    .width(136)
    .height(44)
    .justifyContent(FlexAlign.Center)
    .alignItems(VerticalAlign.Center)
    .borderRadius(8)
    .backgroundColor(HomeTheme.primaryButtonBackground(this.isDark, this.newDeviceHovered, this.newDevicePressed))
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
        .fillColor(SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE))
        .draggable(false)
      TextInput({ placeholder: HomeText.DEVICE_SEARCH_PLACEHOLDER, text: this.profileSearchText })
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
    .width('94%')
    .height(58)
    .padding({ left: 16, right: 10 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(HomeTheme.searchBackground(this.isDark))
    .borderRadius(8)
    .border({
      width: 1,
      color: HomeTheme.searchBorderColor(this.isDark),
      style: BorderStyle.Solid
    })
  }

  @Builder
  private buildDeviceTag(text: string) {
    Text(text)
      .fontSize(10)
      .fontWeight(FontWeight.Medium)
      .fontColor(SettingsTheme.statusText(this.isDark, 'ok'))
      .height(18)
      .padding({ left: 6, right: 6 })
      .backgroundColor(SettingsTheme.statusBackground(this.isDark, 'ok'))
      .borderRadius(6)
      .maxLines(1)
  }

  @Builder
  private buildDeviceIcon(selected: boolean) {
    Column() {
      Image(HomeResources.MONITOR_ICON)
        .width(20)
        .height(20)
        .fillColor(Color.White)
        .draggable(false)
    }
    .width(36)
    .height(36)
    .borderRadius(18)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .backgroundColor(selected ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) :
      SettingsTheme.accentColor(this.isDark, SettingsAccent.CYAN))
  }

  @Builder
  private buildDeviceCard(profile: WindowsConnectionProfile) {
    Row({ space: 8 }) {
      this.buildDeviceIcon(this.isSelectedProfile(profile))

      Column({ space: 3 }) {
        Text(this.profileName(profile))
          .fontSize(14)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
        Text(this.profileAddress(profile))
          .fontSize(12)
          .fontColor(SettingsTheme.secondaryText(this.isDark))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
        Row({ space: 6 }) {
          if (this.isSelectedProfile(profile)) {
            this.buildDeviceTag(HomeText.DEVICE_TAG_RECENT)
          }
          if (profile.rememberPassword) {
            this.buildDeviceTag(HomeText.DEVICE_TAG_PASSWORD_SAVED)
          }
        }
        .height(18)
        .alignItems(VerticalAlign.Center)
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)

      Image(HomeResources.CHEVRON_RIGHT_ICON)
        .width(18)
        .height(18)
        .fillColor(this.isSelectedProfile(profile) ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) :
          SettingsTheme.mutedText(this.isDark))
        .draggable(false)
    }
    .width('94%')
    .height(72)
    .padding({ left: 10, right: 8 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(this.isSelectedProfile(profile) ?
      HomeTheme.activePanelBackground(this.isDark, this.hoveredProfileId === profile.id,
        this.pressedProfileId === profile.id) :
      HomeTheme.inactivePanelBackground(this.isDark, this.hoveredProfileId === profile.id,
        this.pressedProfileId === profile.id))
    .borderRadius(8)
    .border({
      width: this.isSelectedProfile(profile) ? 2 : 1,
      color: this.isSelectedProfile(profile) ? SettingsTheme.accentColor(this.isDark, SettingsAccent.BLUE) :
        (this.hoveredProfileId === profile.id ? HomeTheme.searchBorderColor(this.isDark) :
          HomeTheme.mutedBorderColor(this.isDark))
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
    .width('94%')
    .height(90)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .borderRadius(8)
    .border({
      width: 1,
      color: HomeTheme.mutedBorderColor(this.isDark)
    })
  }

  build() {
    Column({ space: 20 }) {
      Row() {
        Text(HomeText.DEVICE_LIST_TITLE)
          .fontSize(24)
          .fontWeight(FontWeight.Bold)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .maxLines(1)
          .layoutWeight(1)

        this.buildNewDeviceButton()
      }
      .width('94%')
      .alignItems(VerticalAlign.Center)

      this.buildSearchBox()

      Scroll() {
        Row() {
          Column({ space: 10 }) {
            if (!this.hasConnectionProfiles()) {
              this.buildDeviceEmptyState(HomeText.DEVICE_EMPTY_TITLE, HomeText.DEVICE_EMPTY_BODY)
            } else if (this.visibleProfileCount() === 0) {
              this.buildDeviceEmptyState(HomeText.DEVICE_SEARCH_EMPTY_TITLE, HomeText.DEVICE_SEARCH_EMPTY_BODY)
            } else {
              ForEach(this.connectionProfiles, (profile: WindowsConnectionProfile) => {
                if (this.profileVisible(profile)) {
                  this.buildDeviceCard(profile)
                }
              }, (profile: WindowsConnectionProfile) => profile.id)
            }
          }
          .width('100%')
          .alignItems(HorizontalAlign.Center)
        }
        .width('100%')
        .justifyContent(FlexAlign.Center)
      }
      .width('100%')
      .layoutWeight(1)
      .scrollBar(BarState.Auto)
      .scrollBarWidth(4)
    }
    .width('31%')
    .height('100%')
    .padding({ left: 8, right: 8, top: 24, bottom: 24 })
    .alignItems(HorizontalAlign.Center)
    .backgroundColor(HomeTheme.panelBackground(this.isDark))
    .borderRadius(8)
    .border({
      width: 1,
      color: HomeTheme.mutedBorderColor(this.isDark)
    })
  }
}
