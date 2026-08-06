import { NodeContent } from "@ohos.arkui.node";
@Component
export declare struct RdpSessionPage {
    private surfaceContent;
    @Prop
    noticeTitle: string;
    @Prop
    noticeSubtitle: string;
    @Prop
    remoteLoginWaiting: boolean;
    onSurfaceLoad: () => void;
    onAttachContent: (content: NodeContent) => boolean;
    onDetachContent: () => void;
    private previousKeyboardAvoidMode;
    aboutToAppear(): void;
    aboutToDisappear(): void;
    private buildSessionNotice;
    build(): void;
}
