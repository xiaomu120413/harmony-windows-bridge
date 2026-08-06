export type ConnectionValidationIssue = 'hostRequired' | 'hostTooLong' | 'hostSpaces' | 'hostProtocolOrPath' | 'hostFormat' | 'hostIpFormat' | 'portRequired' | 'portDigits' | 'portRange' | 'usernameRequired' | 'passwordRequired';
export interface ConnectionValidationResult {
    ok: boolean;
    issues: ConnectionValidationIssue[];
    host: string;
    port: string;
    username: string;
    password: string;
}
export declare class RdpConnectionValidator {
    static validate(hostInput: string, portInput: string, usernameInput: string, password: string): ConnectionValidationResult;
    private static validateHost;
    private static validatePort;
    private static isIpv4Candidate;
    private static isValidIpv4Host;
    private static isValidIpv6Host;
    private static isValidDnsHost;
}
