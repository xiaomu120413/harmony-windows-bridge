@Component
export declare struct MuHubApp {
    @State
    private host;
    @State
    private port;
    @State
    private username;
    @State
    private password;
    @State
    private connectionProfiles;
    @State
    private selectedConnectionProfileId;
    @State
    private rememberConnectionPassword;
    @State
    private connectionProfilePasswordLoading;
    private certPolicy;
    @State
    private remoteAccessCode;
    @State
    private remoteAccessCodeGateEnabled;
    @State
    private screenRecordingPermissionGranted;
    @State
    private screenRecordingPermissionBusy;
    @State
    private inputInjectionPermissionGranted;
    @State
    private inputInjectionPermissionBusy;
    @State
    private xrdpServerRunning;
    @State
    private xrdpServerState;
    @State
    private xrdpServerPort;
    @State
    private xrdpServerMessage;
    @State
    private xrdpServerBusy;
    @State
    private showSession;
    @State
    private showSettings;
    @State
    private settingsInitialPageName;
    @State
    private settingsRemoteControlSection;
    @State
    private remoteLoginWaiting;
    @State
    private sessionNoticeTitle;
    @State
    private sessionNoticeSubtitle;
    @State
    private connectionFeedbackText;
    @State
    private connectionFeedbackTone;
    @State
    private layoutMode;
    private readonly deviceCapabilities;
    @StorageLink('settingsSystemDark')
    private systemDark;
    @StorageLink('settingsAppearanceMode')
    private appearanceMode;
    private callbacksRegistered;
    private readonly rdpClientController;
    private readonly rdpSurfaceContentHost;
    private permissionManager;
    private permissionRequestCoordinator;
    private remoteControlCoordinator;
    private connectionProfileCoordinator;
    private connectionProfilesLoaded;
    private connectionProfileSelectionGeneration;
    private pendingConnectionProfileSave;
    private lastConnectionErrorMessage;
    private windowLayoutObserverRegistered;
    private windowSizeLayoutBreakpointCallback;
    private deferUiUpdate;
    aboutToAppear(): void;
    onPageShow(): void;
    onPageHide(): void;
    aboutToDisappear(): void;
    private registerWindowLayoutObserver;
    private unregisterWindowLayoutObserver;
    private getUiAbilityContext;
    private remoteControlServerAvailable;
    private getPermissionManager;
    private getPermissionRequestCoordinator;
    private getRemoteControlCoordinator;
    private getConnectionProfileCoordinator;
    private loadConnectionProfiles;
    private applyConnectionSnapshot;
    private findConnectionProfile;
    private findConnectionProfileIn;
    private applyConnectionProfile;
    private selectConnectionProfile;
    private startNewConnectionProfile;
    private deleteSelectedConnectionProfile;
    private setRememberConnectionPassword;
    private saveConnectionProfile;
    private persistPendingConnectionProfile;
    private clearSelectedConnectionPassword;
    private clearConnectionFeedback;
    private setConnectionFeedback;
    private connectionFailureMessage;
    private errorMessage;
    private validateConnectionForm;
    private xrdpServerDisplayMessages;
    private registerNativeCallbacks;
    private isConnectedNativeState;
    private clearSessionNotice;
    private updateSessionNoticeForNativeState;
    private getAppFilesDir;
    private openRemoteFilesDirectoryFromSettings;
    private openSettingsFromHome;
    private registerNativePermissionCallbacks;
    private refreshScreenRecordingPermissionState;
    private requestScreenRecordingPermissionFromSettings;
    private refreshInputInjectionPermissionState;
    private requestInputInjectionPermissionFromSettings;
    private refreshXrdpServerDiagnostics;
    private refreshRemoteControlReadiness;
    private startXrdpServerFromSettings;
    private setRemoteAccessCodeGateFromSettings;
    private regenerateRemoteAccessCodeFromSettings;
    private ensureXrdpServerStarted;
    private applyRemoteControlSnapshot;
    private connectNative;
    private queueNativeConnect;
    private startNativeConnect;
    private setSessionVisible;
    private isConnected;
    private focusRemoteSurface;
    private releaseActiveInput;
    private getAppearanceMode;
    private isAppDark;
    build(): void;
}
