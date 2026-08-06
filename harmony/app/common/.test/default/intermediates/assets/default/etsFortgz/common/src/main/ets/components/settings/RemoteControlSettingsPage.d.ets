import { XrdpServerStatus } from '../../rdp/XrdpServerController';
@Component
export declare struct RemoteControlSettingsPage {
    onBack: () => void;
    onRemoteAccessCodeGateChange: (enabled: boolean) => string;
    onRemoteAccessCodeRegenerate: () => string;
    onRequestScreenRecordingPermission: () => Promise<boolean>;
    onRefreshScreenRecordingPermission: () => boolean;
    onRequestInputInjectionPermission: () => Promise<boolean>;
    onRefreshInputInjectionPermission: () => boolean;
    onRefreshXrdpServerStatus: () => XrdpServerStatus;
    onStartXrdpServer: () => Promise<XrdpServerStatus>;
    onOpenRemoteFilesDirectory: () => void;
    @Prop
    isDark: boolean;
    @Prop
    targetSection: string;
    @Prop
    remoteAccessCode: string;
    @Prop
    remoteAccessCodeGateEnabled: boolean;
    @Prop
    screenRecordingPermissionGranted: boolean;
    @Prop
    inputInjectionPermissionGranted: boolean;
    @Prop
    xrdpServerRunning: boolean;
    @Prop
    xrdpServerState: string;
    @Prop
    xrdpServerPort: number;
    @Prop
    xrdpServerMessage: string;
    @Prop
    xrdpServerBusy: boolean;
    @Prop
    screenRecordingPermissionBusy: boolean;
    @Prop
    inputInjectionPermissionBusy: boolean;
    @State
    private localAccessCode;
    @State
    private localGateEnabled;
    @State
    private localScreenRecordingGranted;
    @State
    private localInputInjectionGranted;
    @State
    private localXrdpServerRunning;
    @State
    private localXrdpServerState;
    @State
    private localXrdpServerPort;
    @State
    private localXrdpServerMessage;
    @State
    private localXrdpServerBusy;
    private screenPermissionRequestPending;
    private inputPermissionRequestPending;
    aboutToAppear(): void;
    private setGateEnabled;
    private regenerateAccessCode;
    private applyXrdpServerStatus;
    private refreshXrdpServerStatus;
    private startXrdpServer;
    private refreshScreenRecordingState;
    private requestScreenRecordingPermission;
    private refreshInputInjectionState;
    private requestInputInjectionPermission;
    build(): void;
}
