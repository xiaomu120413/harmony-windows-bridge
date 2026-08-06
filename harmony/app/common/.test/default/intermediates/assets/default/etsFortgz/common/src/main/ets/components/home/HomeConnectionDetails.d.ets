import { LayoutMode } from '../../adaptive/WindowLayoutPolicy';
import { SettingsStatusTone } from '../settings/SettingsUi';
@Component
export declare struct HomeConnectionDetails {
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
    @Link
    rememberPassword: boolean;
    @Prop
    passwordLoading: boolean;
    @Prop
    selectedConnectionProfileId: string;
    @Prop
    connectionFeedbackText: string;
    @Prop
    connectionFeedbackTone: SettingsStatusTone;
    @Prop
    isDark: boolean;
    onHostChange: (value: string) => void;
    onPortChange: (value: string) => void;
    onUsernameChange: (value: string) => void;
    onPasswordChange: (value: string) => void;
    onDeleteProfile: () => void;
    onRememberPasswordChange: (remember: boolean) => void;
    onClearPassword: () => void;
    onConnect: () => void;
    @State
    private hoveredAction;
    @State
    private pressedAction;
    @State
    private connectHovered;
    @State
    private connectPressed;
    @State
    private formBreakpoint;
    private hasSelectedProfile;
    private setHoveredAction;
    private buildFieldLabel;
    private buildHostInput;
    private buildPortInput;
    private buildUsernameInput;
    private buildPasswordInput;
    private buildRememberControl;
    private buildConnectButton;
    private buildActionButton;
    private buildDeviceActions;
    private buildConnectionFormFields;
    build(): void;
}
