import { SettingsAccentName } from './SettingsUi';
@Component
export declare struct XrdpServerCard {
    @Prop
    isDark: boolean;
    @Prop
    targetSection: string;
    @Prop
    running: boolean;
    @Prop
    busy: boolean;
    @Prop
    state: string;
    @Prop
    port: number;
    @Prop
    message: string;
    onRefresh: () => void;
    onStart: () => void;
    @State
    private hovered;
    @State
    private pressed;
    private isTarget;
    private setHovered;
    private statusText;
    private actionText;
    private actionDisabled;
    build(): void;
}
@Component
export declare struct RemoteFilesCard {
    @Prop
    isDark: boolean;
    @Prop
    targetSection: string;
    onOpenRemoteFilesDirectory: () => void;
    @State
    private hovered;
    @State
    private pressed;
    private isTarget;
    private setHovered;
    build(): void;
}
@Component
export declare struct RemoteAccessCard {
    @Prop
    isDark: boolean;
    @Prop
    targetSection: string;
    @Prop
    gateEnabled: boolean;
    @Prop
    accessCode: string;
    onGateChange: (enabled: boolean) => void;
    onRegenerate: () => void;
    @State
    private hovered;
    @State
    private pressed;
    private isTarget;
    private setHovered;
    private gateStatusText;
    private accessCodeText;
    private regenerateDisabled;
    build(): void;
}
@Component
export declare struct RemotePermissionCard {
    @Prop
    isDark: boolean;
    @Prop
    targetSection: string;
    @Prop
    section: string;
    @Prop
    title: string;
    @Prop
    description: string;
    @Prop
    grantedText: string;
    @Prop
    missingText: string;
    @Prop
    actionTextValue: string;
    @Prop
    busyText: string;
    @Prop
    icon: Resource;
    @Prop
    accent: SettingsAccentName;
    @Prop
    granted: boolean;
    @Prop
    busy: boolean;
    onRequest: () => void;
    @State
    private hovered;
    @State
    private pressed;
    private isTarget;
    private setHovered;
    private statusText;
    private actionText;
    private actionDisabled;
    build(): void;
}
