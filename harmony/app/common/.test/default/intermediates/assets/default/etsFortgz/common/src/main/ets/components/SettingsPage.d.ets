import { LayoutMode } from '../adaptive/WindowLayoutPolicy';
import { XrdpServerStatus } from '../rdp/XrdpServerController';
@Component
export declare struct SettingsPage {
    @Prop
    layoutMode: LayoutMode;
    onClose: () => void;
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
    initialPageName: string;
    @Prop
    initialRemoteControlSection: string;
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
    @Prop
    remoteControlServerAvailable: boolean;
    @State
    private pageName;
    @State
    private remoteControlTargetSection;
    @State
    private appearanceMode;
    @StorageLink('settingsSystemDark')
    private systemDark;
    aboutToAppear(): void;
    private isDarkMode;
    private xrdpStatusTone;
    private xrdpStatusText;
    private screenStatusTone;
    private screenStatusText;
    private gateStatusTone;
    private gateStatusText;
    private buildDesktopNav;
    private buildCompactTopBar;
    private overviewEntry;
    private buildOverviewPage;
    private buildSettingsContent;
    private buildContentPane;
    private buildCompactLayout;
    private buildExpandedLayout;
    build(): void;
}
