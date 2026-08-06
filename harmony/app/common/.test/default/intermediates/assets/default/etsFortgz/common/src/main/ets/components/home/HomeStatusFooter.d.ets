import { LayoutMode } from '../../adaptive/WindowLayoutPolicy';
@Component
export declare struct HomeStatusFooter {
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
    screenRecordingPermissionGranted: boolean;
    @Prop
    screenRecordingPermissionBusy: boolean;
    @Prop
    inputInjectionPermissionGranted: boolean;
    @Prop
    inputInjectionPermissionBusy: boolean;
    @Prop
    remoteControlServerAvailable: boolean;
    onOpenSettings: (remoteControlSection: string) => void;
    @State
    private hoveredSection;
    @State
    private pressedSection;
    private serviceStatusTone;
    private serviceStatusText;
    private screenStatusTone;
    private screenStatusText;
    private inputStatusTone;
    private inputStatusText;
    private cardValue;
    private cardTone;
    private visibleStatusCards;
    private setHoveredSection;
    private buildStatusCard;
    private buildExpandedFooter;
    private buildCompactFooter;
    build(): void;
}
