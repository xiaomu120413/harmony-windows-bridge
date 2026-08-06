import { LayoutMode } from '../../adaptive/WindowLayoutPolicy';
import { SettingsStatusTone } from '../settings/SettingsUi';
import { WindowsConnectionProfile } from '../../rdp/WindowsConnectionStore';
@Component
export declare struct HomePage {
    @Prop
    layoutMode: LayoutMode;
    @Link
    host: string;
    @Link
    port: string;
    @Link
    username: string;
    @Link
    password: string;
    @Prop
    connectionProfiles: WindowsConnectionProfile[];
    @Prop
    selectedConnectionProfileId: string;
    @Link
    rememberPassword: boolean;
    @Prop
    passwordLoading: boolean;
    @Prop
    connectionFeedbackText: string;
    @Prop
    connectionFeedbackTone: SettingsStatusTone;
    @Prop
    isDark: boolean;
    @Prop
    xrdpServerBusy: boolean;
    @Prop
    xrdpServerState: string;
    @Prop
    xrdpServerRunning: boolean;
    @Prop
    screenRecordingPermissionGranted: boolean;
    @Prop
    screenRecordingPermissionBusy: boolean;
    @Prop
    inputInjectionPermissionGranted: boolean;
    @Prop
    inputInjectionPermissionBusy: boolean;
    @Prop
    remoteControlServerAvailable: boolean;
    @State
    private compactPage;
    onHostChange: (value: string) => void;
    onPortChange: (value: string) => void;
    onUsernameChange: (value: string) => void;
    onPasswordChange: (value: string) => void;
    onProfileSelect: (profileId: string) => void;
    onNewProfile: () => void;
    onDeleteProfile: () => void;
    onRememberPasswordChange: (remember: boolean) => void;
    onClearPassword: () => void;
    onConnect: () => void;
    onOpenSettings: (remoteControlSection: string) => void;
    private selectProfile;
    private createProfile;
    private buildDeviceList;
    private buildConnectionDetails;
    private buildExpandedMain;
    private buildCompactMain;
    build(): void;
}
