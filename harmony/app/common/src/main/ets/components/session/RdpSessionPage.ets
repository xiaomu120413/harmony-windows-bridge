import { KeyboardAvoidMode, NodeContent } from '@kit.ArkUI'

@Component
export struct RdpSessionPage {
  private surfaceContent: NodeContent = new NodeContent()
  @Prop noticeTitle: string = ''
  @Prop noticeSubtitle: string = ''
  @Prop remoteLoginWaiting: boolean = false
  onSurfaceLoad: () => void = () => {
  }
  onAttachContent: (content: NodeContent) => boolean = (_content: NodeContent): boolean => false
  onDetachContent: () => void = (): void => {
  }
  private previousKeyboardAvoidMode: KeyboardAvoidMode = KeyboardAvoidMode.OFFSET

  aboutToAppear(): void {
    const uiContext = this.getUIContext()
    this.previousKeyboardAvoidMode = uiContext.getKeyboardAvoidMode()
    uiContext.setKeyboardAvoidMode(KeyboardAvoidMode.NONE)
  }

  aboutToDisappear(): void {
    this.onDetachContent()
    this.getUIContext().setKeyboardAvoidMode(this.previousKeyboardAvoidMode)
  }

  @Builder
  private buildSessionNotice() {
    Row() {
      Column() {
        Text(this.noticeTitle)
          .fontSize(14)
          .fontWeight(FontWeight.Medium)
          .fontColor(Color.White)
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        Text(this.noticeSubtitle)
          .fontSize(12)
          .fontColor('rgba(255,255,255,0.72)')
          .margin({ top: 4 })
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .alignItems(HorizontalAlign.Start)
    }
    .padding({ left: 14, right: 14, top: 10, bottom: 10 })
    .margin({ left: 18, top: 18, right: 18 })
    .constraintSize({ maxWidth: 520 })
    .backgroundColor(this.remoteLoginWaiting ? 'rgba(84,64,22,0.88)' : 'rgba(18,22,30,0.82)')
    .border({
      width: 1,
      color: this.remoteLoginWaiting ? 'rgba(255,190,92,0.42)' : 'rgba(255,255,255,0.18)'
    })
    .borderRadius(8)
  }

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      Stack() {
        ContentSlot(this.surfaceContent)
      }
        .width('100%')
        .height('100%')
        .onAppear(() => {
          if (this.onAttachContent(this.surfaceContent)) {
            this.onSurfaceLoad()
          }
        })
        .backgroundColor(Color.Black)
      if (this.noticeTitle.length > 0) {
        this.buildSessionNotice()
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.Black)
  }
}
