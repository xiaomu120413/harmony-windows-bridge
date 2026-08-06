import common from "@ohos.app.ability.common";
export interface WindowsConnectionProfile {
    id: string;
    name: string;
    host: string;
    port: string;
    username: string;
    certPolicy: string;
    rememberPassword: boolean;
    passwordAlias: string;
    createdAt: number;
    updatedAt: number;
    lastConnectedAt: number;
}
export interface WindowsConnectionSaveInput {
    profileId: string;
    host: string;
    port: string;
    username: string;
    password: string;
    certPolicy: string;
    rememberPassword: boolean;
}
export interface WindowsConnectionSnapshot {
    profiles: WindowsConnectionProfile[];
    selectedProfileId: string;
    selectedPassword: string;
}
export declare class WindowsConnectionStore {
    private readonly context;
    private preferencesCache;
    constructor(context: common.Context);
    loadSnapshot(): Promise<WindowsConnectionSnapshot>;
    loadPassword(profile: WindowsConnectionProfile): Promise<string>;
    saveProfile(input: WindowsConnectionSaveInput): Promise<WindowsConnectionProfile | null>;
    deleteProfile(profileId: string): Promise<WindowsConnectionSnapshot>;
    clearPassword(profileId: string): Promise<WindowsConnectionSnapshot>;
    private getPreferences;
    private loadProfiles;
    private loadProfileIds;
    private loadProfile;
    private resolveSelectedProfileId;
    private persistProfiles;
    private deleteProfileKey;
    private applyPasswordPreference;
    private savePassword;
    private loadPasswordForProfile;
    private removePassword;
    private static createProfile;
    private static updatedProfile;
    private static sanitizeProfile;
    private static findProfileForSave;
    private static findProfileById;
    private static upsertProfile;
    private static sortProfiles;
    private static firstProfileId;
    private static normalizePort;
    private static profileName;
    private static profileKey;
    private static passwordAlias;
    private static newProfileId;
    private static safeTimestamp;
    private static stringToBytes;
    private static bytesToString;
}
