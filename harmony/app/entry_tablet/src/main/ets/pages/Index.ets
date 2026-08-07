import { MuHubApp } from 'common'

@Entry
@Component
struct Index {
  build() {
    Column() {
      MuHubApp()
    }
    .width('100%')
    .height('100%')
  }
}
