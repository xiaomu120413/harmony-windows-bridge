import { AbilityConstant, Configuration, UIAbility, Want } from '@kit.AbilityKit'
import { window } from '@kit.ArkUI'
import {
  ImeHostWindowBinder,
  packagingProbe,
  RemoteFilesDirectory,
  SettingsTheme
} from 'common'

const TAG = 'EntryAbility'

export default class EntryAbility extends UIAbility {
  private readonly imeHostWindowBinder: ImeHostWindowBinder = new ImeHostWindowBinder()

  onCreate(_want: Want, _launchParam: AbilityConstant.LaunchParam): void {
    console.info(`${TAG} ${packagingProbe('2in1')}`)
    SettingsTheme.initAppearance(this.context)
    if (!SettingsTheme.applyAppearanceModeToApplication(this.context, SettingsTheme.getStoredAppearanceMode())) {
      console.error(`${TAG} setColorMode failed`)
    }
  }

  onDestroy(): void {
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    windowStage.loadContent('pages/Index', (err) => {
      if (err.code) {
        console.error(`${TAG} loadContent failed: ${JSON.stringify(err)}`)
        return
      }
      const mainWindow = windowStage.getMainWindowSync()
      this.imeHostWindowBinder.bind(mainWindow)
      this.ensureDownloadDriveDirectory()
    })
  }

  onWindowStageDestroy(): void {
  }

  onForeground(): void {
  }

  onBackground(): void {
  }

  onConfigurationUpdate(newConfig: Configuration): void {
    SettingsTheme.updateSystemAppearance(newConfig?.colorMode)
  }

  private ensureDownloadDriveDirectory(): void {
    RemoteFilesDirectory.prepareSharedDirectory(this.context)
      .catch((error: Error) => {
        console.error(`${TAG} prepare download drive directory failed: ${JSON.stringify(error)}`)
      })
  }
}
