import { AbilityConstant, Configuration, UIAbility, Want } from '@kit.AbilityKit'
import { window } from '@kit.ArkUI'
import { fileIo, fileUri, picker } from '@kit.CoreFileKit'
import { SettingsTheme } from '../components/settings/SettingsUi'

const TAG = 'EntryAbility'
const RDP_DOWNLOAD_DRIVE_DIR_NAME = 'com.muhub.desktop'

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

  private async ensureDownloadDriveDirectory(): Promise<void> {
    try {
      const pickedDir = await this.pickDownloadDirectory()
      if (pickedDir.length === 0) {
        console.error(`${TAG} prepare download drive directory skipped: Download picker returned empty path`)
        return
      }

      const targetDir = this.endsWithPathComponent(pickedDir, RDP_DOWNLOAD_DRIVE_DIR_NAME) ?
        pickedDir : this.joinPath(pickedDir, RDP_DOWNLOAD_DRIVE_DIR_NAME)
      if (this.isDirectory(targetDir)) {
        console.info(`${TAG} download drive directory ready: ${targetDir}`)
        return
      }

      fileIo.mkdirSync(targetDir)
      console.info(`${TAG} download drive directory created: ${targetDir}`)
    } catch (error) {
      console.error(`${TAG} prepare download drive directory failed: ${JSON.stringify(error)}`)
    }
  }

  private async pickDownloadDirectory(): Promise<string> {
    const documentViewPicker = new picker.DocumentViewPicker(this.context)
    const saveOptions = new picker.DocumentSaveOptions()
    saveOptions.pickerMode = picker.DocumentPickerMode.DOWNLOAD
    const result = await documentViewPicker.save(saveOptions)
    if (result.length === 0) {
      return ''
    }
    return new fileUri.FileUri(result[0]).path
  }

  private isDirectory(path: string): boolean {
    try {
      return fileIo.statSync(path).isDirectory()
    } catch (_error) {
      return false
    }
  }

  private joinPath(parent: string, child: string): string {
    return parent.endsWith('/') ? `${parent}${child}` : `${parent}/${child}`
  }

  private endsWithPathComponent(path: string, component: string): boolean {
    return path === component || path.endsWith(`/${component}`)
  }
}
