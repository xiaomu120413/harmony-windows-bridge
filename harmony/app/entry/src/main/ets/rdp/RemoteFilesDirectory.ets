import { common, Want, wantConstant } from '@kit.AbilityKit'
import { Environment, fileIo, fileUri } from '@kit.CoreFileKit'
import {
  REMOTE_FILES_SHARED_DIR_NAME,
  REMOTE_FILES_SHARED_DIR_STORAGE_KEY
} from '../components/settings/SettingsConstants'
import { RdpLogger } from './RdpLogger'

const FILE_MANAGER_BUNDLE_NAME = 'com.huawei.hmos.filemanager'
const FILE_MANAGER_MAIN_ABILITY = 'MainAbility'
const FILE_MANAGER_OPEN_DIRECTORY_MODE = 'openDirectory'
const FILE_MANAGER_OPEN_DIRECTORY_URI = 'filemanager://openDirectory'

export class RemoteFilesDirectory {
  static prepareSharedDirectory(): void {
    try {
      const downloadDir = Environment.getUserDownloadDir()
      if (downloadDir.length === 0) {
        RdpLogger.warn('prepare download drive directory skipped: Download directory is empty')
        return
      }

      const targetDir = RemoteFilesDirectory.sharedDirectoryPath(downloadDir)
      if (RemoteFilesDirectory.isDirectory(targetDir)) {
        AppStorage.setOrCreate<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY, targetDir)
        return
      }

      fileIo.mkdirSync(targetDir)
      AppStorage.setOrCreate<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY, targetDir)
    } catch (exception) {
      RdpLogger.error(`prepare download drive directory failed: ${JSON.stringify(exception)}`)
    }
  }

  static async openSharedDirectory(context: common.UIAbilityContext | null): Promise<void> {
    if (context === null) {
      RdpLogger.warn('remote files directory open skipped: no UIAbilityContext')
      return
    }

    const storedDir = AppStorage.get<string>(REMOTE_FILES_SHARED_DIR_STORAGE_KEY) || ''
    if (!RemoteFilesDirectory.isDirectory(storedDir)) {
      RdpLogger.warn('remote files directory open skipped: shared directory is not ready')
      return
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

  private static sharedDirectoryPath(downloadDir: string): string {
    if (RemoteFilesDirectory.endsWithPathComponent(downloadDir, REMOTE_FILES_SHARED_DIR_NAME)) {
      return downloadDir
    }
    return RemoteFilesDirectory.joinPath(downloadDir, REMOTE_FILES_SHARED_DIR_NAME)
  }

  private static isDirectory(path: string): boolean {
    try {
      return path.length > 0 && fileIo.statSync(path).isDirectory()
    } catch (_exception) {
      return false
    }
  }

  private static joinPath(parent: string, child: string): string {
    return parent.endsWith('/') ? `${parent}${child}` : `${parent}/${child}`
  }

  private static endsWithPathComponent(path: string, component: string): boolean {
    return path === component || path.endsWith(`/${component}`)
  }
}
