import { AbilityConstant, Configuration, UIAbility, Want } from '@kit.AbilityKit'
import { window } from '@kit.ArkUI'
import { SettingsTheme } from '../components/settings/SettingsUi'
import { RemoteFilesDirectory } from '../rdp/RemoteFilesDirectory'
import { HostWindowTracker } from '../rdp/HostWindowTracker'

const TAG = 'EntryAbility'

export default class EntryAbility extends UIAbility {
  private readonly hostWindowTracker: HostWindowTracker = new HostWindowTracker()

  onCreate(_want: Want, _launchParam: AbilityConstant.LaunchParam): void {
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
      this.hostWindowTracker.start(mainWindow)
      this.ensureDownloadDriveDirectory()
    })
  }

  onWindowStageDestroy(): void {
    this.hostWindowTracker.stop()
  }

  onForeground(): void {
  }

  onBackground(): void {
  }

  onConfigurationUpdate(newConfig: Configuration): void {
    SettingsTheme.updateSystemAppearance(newConfig?.colorMode)
  }

  private ensureDownloadDriveDirectory(): void {
    RemoteFilesDirectory.prepareSharedDirectory()
  }
}
