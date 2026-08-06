import { SettingsAppearanceMode } from './SettingsUi';
@Component
export declare struct BasicSettingsPage {
    onBack: () => void;
    onModeChange: (mode: SettingsAppearanceMode) => void;
    @State
    private localMode;
    @StorageLink('settingsSystemDark')
    private systemDark;
    @State
    private interfaceName;
    @State
    private ipAddresses;
    @State
    private networkCardHovered;
    @State
    private networkCardPressed;
    aboutToAppear(): void;
    private resolveVisualDark;
    private isVisualDark;
    private applyColorMode;
    private setNetworkCardHovered;
    private refreshIp;
    private modeOption;
    private networkInfoCard;
    build(): void;
}
