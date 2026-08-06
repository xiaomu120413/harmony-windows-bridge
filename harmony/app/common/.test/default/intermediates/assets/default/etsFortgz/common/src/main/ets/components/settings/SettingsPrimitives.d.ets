import { SettingsAccentName, SettingsStatusTone } from './SettingsTypes';
@Component
export declare struct SettingsStatusChip {
    @Prop
    text: string;
    @Prop
    tone: SettingsStatusTone;
    @Prop
    isDark: boolean;
    build(): void;
}
@Component
export declare struct SettingsDesktopNavItem {
    @Prop
    title: string;
    @Prop
    iconResource: Resource;
    @Prop
    accentName: SettingsAccentName;
    @Prop
    isDark: boolean;
    @Prop
    selected: boolean;
    onPress: () => void;
    @State
    private hovered;
    @State
    private pressed;
    private setHovered;
    build(): void;
}
@Component
export declare struct SettingsKeyValueRow {
    @Prop
    label: string;
    @Prop
    value: string;
    @Prop
    isDark: boolean;
    @Prop
    tone: SettingsStatusTone;
    build(): void;
}
@Component
export declare struct SettingsBackButton {
    @Prop
    isDark: boolean;
    onBack: () => void;
    @State
    private hovered;
    @State
    private pressed;
    private setHovered;
    build(): void;
}
@Component
export declare struct SettingsPageHeader {
    @Prop
    title: string;
    @Prop
    subtitle: string;
    @Prop
    isDark: boolean;
    onBack: () => void;
    build(): void;
}
@Component
export declare struct SettingsListItem {
    @Prop
    title: string;
    @Prop
    description: string;
    @Prop
    iconResource: Resource;
    @Prop
    accentName: SettingsAccentName;
    @Prop
    value: string;
    @Prop
    isDark: boolean;
    @Prop
    selected: boolean;
    onPress: () => void;
    @State
    private hovered;
    @State
    private pressed;
    private setHovered;
    build(): void;
}
@Component
export declare struct SettingsSectionTitle {
    @Prop
    title: string;
    @Prop
    subtitle: string;
    @Prop
    isDark: boolean;
    build(): void;
}
@Component
export declare struct SettingsInfoCard {
    @Prop
    title: string;
    @Prop
    body: string;
    @Prop
    isDark: boolean;
    @State
    private hovered;
    @State
    private pressed;
    private setHovered;
    build(): void;
}
