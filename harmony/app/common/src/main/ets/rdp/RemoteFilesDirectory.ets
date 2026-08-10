import { common, Want, wantConstant } from '@kit.AbilityKit'
import { fileIo, fileUri, picker } from '@kit.CoreFileKit'
import {
  REMOTE_FILES_SHARED_DIR_STORAGE_KEY
} from '../components/settings/SettingsConstants'
import { RdpLogger } from './RdpLogger'

const FILE_MANAGER_BUNDLE_NAME = 'com.huawei.hmos.filemanager'
const FILE_MANAGER_MAIN_ABILITY = 'MainAbility'
const FILE_MANAGER_OPEN_DIRECTORY_MODE = 'openDirectory'
const FILE_MANAGER_OPEN_DIRECTORY_URI = 'filemanager://openDirectory'

export class RemoteFilesDirectory {
  static sharedDirectoryPath(): string {
    const storedDir = AppStorage.get<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY) || ''
    return RemoteFilesDirectory.isDirectory(storedDir) ? storedDir : ''
  }

  static async prepareSharedDirectory(context: common.UIAbilityContext | null): Promise<boolean> {
    if (context === null) {
      RdpLogger.warn('prepare download drive directory skipped: no UIAbilityContext')
      return false
    }

    const storedDir = AppStorage.get<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY) || ''
    if (RemoteFilesDirectory.isDirectory(storedDir)) {
      return true
    }

    try {
      const options = new picker.DocumentSaveOptions()
      options.pickerMode = picker.DocumentPickerMode.DOWNLOAD
      const selectedUris = await new picker.DocumentViewPicker(context).save(options)
      if (selectedUris.length === 0) {
        RdpLogger.warn('prepare download drive directory skipped: picker returned no directory')
        return false
      }

      const targetDir = new fileUri.FileUri(selectedUris[0]).path
      if (!RemoteFilesDirectory.isDirectory(targetDir)) {
        RdpLogger.warn('prepare download drive directory failed: picker result is not a directory')
        return false
      }

      AppStorage.setOrCreate<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY, targetDir)
      RdpLogger.info('prepare download drive directory succeeded')
      return true
    } catch (exception) {
      RdpLogger.error(`prepare download drive directory failed: ${JSON.stringify(exception)}`)
      return false
    }
  }

  static async openSharedDirectory(context: common.UIAbilityContext | null): Promise<void> {
    if (context === null) {
      RdpLogger.warn('remote files directory open skipped: no UIAbilityContext')
      return
    }

    let storedDir = AppStorage.get<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY) || ''
    if (!RemoteFilesDirectory.isDirectory(storedDir)) {
      if (!await RemoteFilesDirectory.prepareSharedDirectory(context)) {
        RdpLogger.warn('remote files directory open skipped: shared directory is not ready')
        return
      }
      storedDir = AppStorage.get<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY) || ''
    }

    try {
      await context.startAbility(RemoteFilesDirectory.openDirectoryWant(storedDir))
    } catch (exception) {
      RdpLogger.error(`remote files directory startAbility failed: ${JSON.stringify(exception)}`)
    }
  }

  private static openDirectoryWant(path: string): Want {
    const directoryUri = new fileUri.FileUri(path).toString()
    const parametersObject: Object = new Object()
    Reflect.set(parametersObject, 'fileManagerMode', FILE_MANAGER_OPEN_DIRECTORY_MODE)
    Reflect.set(parametersObject, 'fileUri', directoryUri)

    return {
      bundleName: FILE_MANAGER_BUNDLE_NAME,
      abilityName: FILE_MANAGER_MAIN_ABILITY,
      action: 'ohos.want.action.viewData',
      uri: FILE_MANAGER_OPEN_DIRECTORY_URI,
      flags: wantConstant.Flags.FLAG_AUTH_READ_URI_PERMISSION | wantConstant.Flags.FLAG_AUTH_WRITE_URI_PERMISSION,
      parameters: parametersObject as Record<string, Object>
    }
  }

  private static isDirectory(path: string): boolean {
    try {
      return path.length > 0 && fileIo.statSync(path).isDirectory()
    } catch (_exception) {
      return false
    }
  }

}
