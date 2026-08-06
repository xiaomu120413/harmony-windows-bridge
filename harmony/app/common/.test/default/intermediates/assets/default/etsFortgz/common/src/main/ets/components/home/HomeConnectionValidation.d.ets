import { ConnectionValidationIssue } from '../../rdp/RdpConnectionValidator';
export declare class HomeConnectionValidation {
    static message(issues: ConnectionValidationIssue[]): string;
    private static issueMessage;
}
