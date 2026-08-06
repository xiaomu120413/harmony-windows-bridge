import common from "@ohos.app.ability.common";
export declare class RemoteFilesDirectory {
    static prepareSharedDirectory(): void;
    static openSharedDirectory(context: common.UIAbilityContext | null): Promise<void>;
    private static openDirectoryWant;
    private static sharedDirectoryPath;
    private static isDirectory;
    private static joinPath;
    private static endsWithPathComponent;
}
