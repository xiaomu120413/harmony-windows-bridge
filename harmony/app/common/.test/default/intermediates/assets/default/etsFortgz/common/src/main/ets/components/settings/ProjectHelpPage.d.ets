@Component
export declare struct ProjectHelpPage {
    onBack: () => void;
    @Prop
    isDark: boolean;
    @State
    private aboutCardHovered;
    @State
    private aboutCardPressed;
    private setAboutCardHovered;
    private isUrl;
    private aboutInfoRow;
    private aboutInfoCard;
    build(): void;
}
