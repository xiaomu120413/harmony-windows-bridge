import common from "@ohos.app.ability.common";
export interface RdpPermissionDelegate {
    getContext(): common.UIAbilityContext | null;
}
export declare class RdpPermissionCallbackDelegate implements RdpPermissionDelegate {
    private readonly getContextCallback;
    constructor(getContext: () => common.UIAbilityContext | null);
    getContext(): common.UIAbilityContext | null;
}
type PermissionPromptOpenedCallback = () => void;
export declare class RdpPermissionManager {
    private delegate;
    private readonly onScreenRecordingPermissionPromptOpen;
    private pasteboardPermission;
    private microphonePermission;
    private cameraPermission;
    private locationPermission;
    private screenRecordingPermission;
    private controlDevicePermission;
    private screenRecordingPermissionPromptDelayTimer;
    private screenRecordingPermissionPromptDelayResolve;
    constructor(delegate: RdpPermissionDelegate, onScreenRecordingPermissionPromptOpen?: PermissionPromptOpenedCallback);
    ensurePasteboardReadPermission(trigger: string): Promise<boolean>;
    ensureMicrophonePermission(trigger: string): Promise<boolean>;
    ensureCameraPermission(trigger: string): Promise<boolean>;
    ensureLocationPermission(trigger: string): Promise<boolean>;
    ensureScreenRecordingPermission(trigger: string): Promise<boolean>;
    requestScreenRecordingPermission(trigger: string): Promise<boolean>;
    hasScreenRecordingPermission(): boolean;
    requestControlDevicePermission(trigger: string): Promise<boolean>;
    hasControlDevicePermission(): boolean;
    private ensurePermission;
    private getSelfAccessTokenId;
    private hasPermission;
    private openPermissionOnSetting;
    private pollPermissionOnSettingResult;
    private openScreenRecordingPermissionOnSetting;
    private openScreenRecordingPermissionOnSettingAfterDelay;
    private openPendingScreenRecordingPermissionPrompt;
    private clearScreenRecordingPermissionPromptDelay;
    private requestRuntimePermissions;
    private hasLocationPermission;
}
export {};
