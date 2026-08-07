import { MuHubApp } from 'common'
import { entryRemoteControlPort } from '../rdp/EntryRemoteControlPort'

@Entry
@Component
struct Index {
  build() {
    Column() {
      MuHubApp({ remoteControlPort: entryRemoteControlPort })
    }
    .width('100%')
    .height('100%')
  }
}
