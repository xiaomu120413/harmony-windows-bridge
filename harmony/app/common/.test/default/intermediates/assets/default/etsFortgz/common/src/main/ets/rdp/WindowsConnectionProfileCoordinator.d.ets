import common from "@ohos.app.ability.common";
import { WindowsConnectionProfile, WindowsConnectionSaveInput, WindowsConnectionSnapshot } from './WindowsConnectionStore';
export declare class WindowsConnectionProfileCoordinator {
    private readonly store;
    constructor(context: common.Context);
    load(): Promise<WindowsConnectionSnapshot | null>;
    loadPassword(profile: WindowsConnectionProfile): Promise<string | null>;
    deleteProfile(profileId: string): Promise<WindowsConnectionSnapshot | null>;
    saveProfile(input: WindowsConnectionSaveInput): Promise<WindowsConnectionSnapshot | null>;
    clearPassword(profileId: string): Promise<WindowsConnectionSnapshot | null>;
}
