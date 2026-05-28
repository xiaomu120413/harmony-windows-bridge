import { AbilityConstant, Configuration, UIAbility, Want } from '@kit.AbilityKit'
import { window } from '@kit.ArkUI'
import { SettingsTheme } from '../components/settings/SettingsUi'
import { RemoteFilesDirectory } from '../rdp/RemoteFilesDirectory'

const TAG = 'EntryAbility'

export default class EntryAbility extends UIAbility {
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
    RemoteFilesDirectory.prepareSharedDirectory()
  }
}
