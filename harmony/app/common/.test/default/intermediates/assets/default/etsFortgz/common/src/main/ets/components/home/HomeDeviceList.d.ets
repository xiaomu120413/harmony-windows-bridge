import { LayoutMode } from '../../adaptive/WindowLayoutPolicy';
import { WindowsConnectionProfile } from '../../rdp/WindowsConnectionStore';
@Component
export declare struct HomeDeviceList {
    @Prop
    layoutMode: LayoutMode;
    @Prop
    connectionProfiles: WindowsConnectionProfile[];
    @Prop
    selectedConnectionProfileId: string;
    @Prop
    isDark: boolean;
    onProfileSelect: (profileId: string) => void;
    onNewProfile: () => void;
    @State
    private profileSearchText;
    @State
    private hoveredProfileId;
    @State
    private pressedProfileId;
    @State
    private newDeviceHovered;
    @State
    private newDevicePressed;
    private hasConnectionProfiles;
    private isSelectedProfile;
    private profileName;
    private profileAddress;
    private normalizedSearchText;
    private profileVisible;
    private visibleProfileCount;
    private setHoveredProfile;
    private buildNewDeviceButton;
    private buildSearchBox;
    private buildDeviceTag;
    private buildDeviceIcon;
    private buildDeviceCard;
    private buildDeviceEmptyState;
    build(): void;
}
