import { SettingsAccent, SettingsResources } from './SettingsResources'
import { SettingsTheme } from './SettingsTheme'
import { SettingsAccentName, SettingsStatusTone } from './SettingsTypes'

@Component
export struct SettingsStatusChip {
  @Prop text: string = ''
  @Prop tone: SettingsStatusTone = 'neutral'
  @Prop isDark: boolean = false

  build() {
    Row({ space: 6 }) {
      Circle()
        .width(7)
        .height(7)
        .fill(SettingsTheme.statusText(this.isDark, this.tone))

      Text(this.text)
        .fontSize(11)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.statusText(this.isDark, this.tone))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
    }
    .height(28)
    .padding({ left: 10, right: 10 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor(SettingsTheme.statusBackground(this.isDark, this.tone))
    .borderRadius(14)
    .border({
      width: 1,
      color: SettingsTheme.statusBorder(this.isDark, this.tone)
    })
  }
}

@Component
export struct SettingsDesktopNavItem {
  @Prop title: string = ''
  @Prop iconResource: Resource = SettingsResources.SETTINGS_ICON
  @Prop accentName: SettingsAccentName = SettingsAccent.BLUE
  @Prop isDark: boolean = false
  @Prop selected: boolean = false
  onPress: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    this.hovered = hovered
  }

  build() {
    Row({ space: 10 }) {
      Column() {
        Image(this.iconResource)
          .width(17)
          .height(17)
          .fillColor(this.selected ? Color.White : SettingsTheme.accentColor(this.isDark, this.accentName))
          .draggable(false)
      }
      .width(32)
      .height(32)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(10)
      .backgroundColor(this.selected ? SettingsTheme.accentColor(this.isDark, this.accentName) :
        SettingsTheme.accentBackground(this.isDark, this.accentName))

      Text(this.title)
        .fontSize(14)
        .fontWeight(this.selected ? FontWeight.Bold : FontWeight.Medium)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .layoutWeight(1)
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .height(46)
    .padding({ left: 10, right: 10 })
    .backgroundColor(SettingsTheme.transparentButton(this.selected || this.hovered, this.pressed, this.isDark))
    .borderRadius(12)
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
    .onClick(() => {
      this.pressed = false
      this.onPress()
    })
  }
}

@Component
export struct SettingsKeyValueRow {
  @Prop label: string = ''
  @Prop value: string = ''
  @Prop isDark: boolean = false
  @Prop tone: SettingsStatusTone = 'neutral'

  build() {
    Row() {
      Text(this.label)
        .fontSize(12)
        .fontColor(SettingsTheme.mutedText(this.isDark))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .layoutWeight(1)

      Text(this.value)
        .fontSize(13)
        .fontWeight(FontWeight.Medium)
        .fontColor(this.tone === 'neutral' ? SettingsTheme.primaryText(this.isDark) :
          SettingsTheme.statusText(this.isDark, this.tone))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .textAlign(TextAlign.End)
        .margin({ left: 12 })
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .height(32)
  }
}

@Component
export struct SettingsBackButton {
  @Prop isDark: boolean = false
  onBack: () => void = () => {
  }
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    this.hovered = hovered
  }

  build() {
    Button() {
      Image(SettingsResources.ARROW_LEFT_ICON)
        .width(18)
        .height(18)
        .fillColor(SettingsTheme.primaryText(this.isDark))
        .draggable(false)
    }
    .type(ButtonType.Normal)
    .width(48)
    .height(48)
    .borderRadius(SettingsTheme.BUTTON_RADIUS)
    .backgroundColor(SettingsTheme.transparentButton(this.hovered, this.pressed, this.isDark))
    .stateEffect(false)
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
    .onClick(() => {
      this.pressed = false
      this.onBack()
    })
  }
}

@Component
export struct SettingsPageHeader {
  @Prop title: string = ''
  @Prop subtitle: string = ''
  @Prop isDark: boolean = false
  onBack: () => void = () => {
  }

  build() {
    Row() {
      Row({ space: 8 }) {
        SettingsBackButton({
          isDark: this.isDark,
          onBack: this.onBack
        })

        Column({ space: 4 }) {
          Text(this.title)
            .fontSize(24)
            .fontWeight(FontWeight.Bold)
            .fontColor(SettingsTheme.primaryText(this.isDark))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')

          if (this.subtitle.length > 0) {
            Text(this.subtitle)
              .fontSize(12)
              .fontColor(SettingsTheme.mutedText(this.isDark))
              .maxLines(2)
              .textOverflow({ overflow: TextOverflow.Ellipsis })
              .width('100%')
          }
        }
        .alignItems(HorizontalAlign.Start)
        .layoutWeight(1)
      }
      .layoutWeight(1)
      .alignItems(VerticalAlign.Top)
    }
    .width('100%')
    .alignItems(VerticalAlign.Top)
    .padding({ left: 16, right: 20, top: 16, bottom: 14 })
  }
}

@Component
export struct SettingsListItem {
  @Prop title: string = ''
  @Prop description: string = ''
  @Prop iconResource: Resource = SettingsResources.SETTINGS_ICON
  @Prop accentName: SettingsAccentName = SettingsAccent.BLUE
  @Prop value: string = ''
  @Prop isDark: boolean = false
  @Prop selected: boolean = false
  onPress: () => void = () => {
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
        Image(this.iconResource)
          .width(SettingsTheme.ICON_SIZE)
          .height(SettingsTheme.ICON_SIZE)
          .fillColor(this.selected ? Color.White : SettingsTheme.accentColor(this.isDark, this.accentName))
          .draggable(false)
      }
      .width(SettingsTheme.ICON_TILE_SIZE)
      .height(SettingsTheme.ICON_TILE_SIZE)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(SettingsTheme.BUTTON_RADIUS)
      .backgroundColor(this.selected ? SettingsTheme.accentColor(this.isDark, this.accentName) :
        (this.hovered || this.pressed ? SettingsTheme.accentHoverBackground(this.isDark, this.accentName) :
          SettingsTheme.accentBackground(this.isDark, this.accentName)))

      Column({ space: 5 }) {
        Text(this.title)
          .fontSize(15)
          .fontWeight(this.selected ? FontWeight.Bold : FontWeight.Medium)
          .fontColor(SettingsTheme.primaryText(this.isDark))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
        Text(this.description)
          .fontSize(13)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .maxLines(2)
          .lineHeight(19)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)

      if (this.selected) {
        Image(SettingsResources.CHECK_ICON)
          .width(18)
          .height(18)
          .fillColor(SettingsTheme.accentColor(this.isDark, this.accentName))
          .margin({ left: 6 })
          .draggable(false)
      } else if (this.value.length > 0) {
        Text(this.value)
          .fontSize(12)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .margin({ left: 6 })
      }
    }
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .padding({ left: 14, right: 14, top: 12, bottom: 12 })
    .backgroundColor(this.hovered || this.pressed ? SettingsTheme.cardHoverBackground(this.isDark) :
      SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: this.selected ? SettingsTheme.accentColor(this.isDark, this.accentName) :
        SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered))
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
    .onClick(() => {
      this.pressed = false
      this.onPress()
    })
  }
}

@Component
export struct SettingsSectionTitle {
  @Prop title: string = ''
  @Prop subtitle: string = ''
  @Prop isDark: boolean = false

  build() {
    Column({ space: 4 }) {
      Text(this.title)
        .fontSize(13)
        .fontWeight(FontWeight.Bold)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width('100%')
      if (this.subtitle.length > 0) {
        Text(this.subtitle)
          .fontSize(12)
          .fontColor(SettingsTheme.mutedText(this.isDark))
          .width('100%')
          .lineHeight(18)
      }
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .margin({ top: 6, bottom: 10 })
  }
}

@Component
export struct SettingsInfoCard {
  @Prop title: string = ''
  @Prop body: string = ''
  @Prop isDark: boolean = false
  @State private hovered: boolean = false
  @State private pressed: boolean = false

  private setHovered(hovered: boolean): void {
    SettingsTheme.animate(this.getUIContext(), () => {
      this.hovered = hovered
    })
  }

  build() {
    Column() {
      Text(this.title)
        .fontSize(15)
        .fontWeight(FontWeight.Medium)
        .fontColor(SettingsTheme.primaryText(this.isDark))
        .width('100%')
        .margin({ bottom: 8 })
      Text(this.body)
        .fontSize(14)
        .fontColor(SettingsTheme.secondaryText(this.isDark))
        .lineHeight(22)
        .width('100%')
    }
    .alignItems(HorizontalAlign.Start)
    .width('100%')
    .padding(16)
    .backgroundColor(this.hovered || this.pressed ? SettingsTheme.cardHoverBackground(this.isDark) :
      SettingsTheme.cardBackground(this.isDark))
    .borderRadius(SettingsTheme.CARD_RADIUS)
    .border({
      width: 1,
      color: SettingsTheme.borderColor(this.isDark)
    })
    .shadow(SettingsTheme.shadow(this.isDark, this.hovered))
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
