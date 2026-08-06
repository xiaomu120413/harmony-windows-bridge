import { LayoutMode } from '../../adaptive/WindowLayoutPolicy';
@Component
export declare struct HomeHeader {
    @Prop
    layoutMode: LayoutMode;
    @Prop
    isDark: boolean;
    @Prop
    xrdpServerBusy: boolean;
    @Prop
    xrdpServerState: string;
    @Prop
    xrdpServerRunning: boolean;
    @Prop
    remoteControlServerAvailable: boolean;
    onOpenSettings: (remoteControlSection: string) => void;
    @State
    private settingsHovered;
    @State
    private settingsPressed;
    private readonly headerSideWidth;
    private headerStatusTone;
    private headerStatusText;
    private buildHeaderStatus;
    private buildSettingsButton;
    private buildExpandedHeader;
    private buildCompactHeader;
    build(): void;
}
