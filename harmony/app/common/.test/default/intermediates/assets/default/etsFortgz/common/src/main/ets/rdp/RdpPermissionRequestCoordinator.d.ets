import { NativeCommandResult } from './NativeRdpGateway';
import { RdpPermissionManager } from './RdpPermissions';
export declare class RdpPermissionRequestCoordinator {
    private readonly permissionManager;
    private readonly deferUiUpdate;
    constructor(permissionManager: RdpPermissionManager, deferUiUpdate: (task: () => void) => void);
    register(): NativeCommandResult;
    private routes;
    private findRoute;
    private handleRequest;
    private complete;
}
