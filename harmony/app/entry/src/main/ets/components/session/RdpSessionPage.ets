import { KeyboardAvoidMode } from '@kit.ArkUI'
import { REMOTE_SURFACE_FOCUS_ID } from '../../rdp/RdpConstants'

@Component
export struct RdpSessionPage {
  surfaceController: XComponentController | undefined = undefined
  @Prop noticeTitle: string = ''
  @Prop noticeSubtitle: string = ''
  @Prop remoteLoginWaiting: boolean = false
  onSurfaceLoad: () => void = () => {
  }
  private previousKeyboardAvoidMode: KeyboardAvoidMode = KeyboardAvoidMode.OFFSET

  aboutToAppear(): void {
    const uiContext = this.getUIContext()
    this.previousKeyboardAvoidMode = uiContext.getKeyboardAvoidMode()
    uiContext.setKeyboardAvoidMode(KeyboardAvoidMode.NONE)
  }

  aboutToDisappear(): void {
    this.getUIContext().setKeyboardAvoidMode(this.previousKeyboardAvoidMode)
  }

  private requireSurfaceController(): XComponentController {
    if (this.surfaceController === undefined) {
      throw new Error('RdpSessionPage requires the existing XComponentController')
    }
    return this.surfaceController
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
      XComponent({
        id: 'rdpSurface',
        type: XComponentType.SURFACE,
        libraryname: 'entry',
        controller: this.requireSurfaceController()
      })
        .width('100%')
        .height('100%')
        .onLoad(() => {
          this.onSurfaceLoad()
        })
        .id(REMOTE_SURFACE_FOCUS_ID)
        .focusable(true)
        .focusOnTouch(true)
        .defaultFocus(true)
        .backgroundColor(Color.Black)
        .renderFit(RenderFit.CENTER)
      if (this.noticeTitle.length > 0) {
        this.buildSessionNotice()
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.Black)
  }
}
