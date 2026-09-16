import { describe, expect, it } from '@ohos/hypium'
import { RdpClientController } from '../main/ets/rdp/RdpClientController'

export default function rdpClientControllerTest(): void {
  describe('RdpClientController', (): void => {
    it('isConnectedStateReturnsTrueForConnectedStates', 0, (): void => {
      expect(RdpClientController.isConnectedState('Connected')).assertTrue()
      expect(RdpClientController.isConnectedState('RemoteLoginWaiting')).assertTrue()
      expect(RdpClientController.isConnectedState('RemoteDesktopReady')).assertTrue()
    })

    it('isConnectedStateReturnsFalseForNonConnectedStates', 0, (): void => {
      expect(RdpClientController.isConnectedState('Disconnected')).assertFalse()
      expect(RdpClientController.isConnectedState('Failed')).assertFalse()
      expect(RdpClientController.isConnectedState('Idle')).assertFalse()
      expect(RdpClientController.isConnectedState('Resolving')).assertFalse()
      expect(RdpClientController.isConnectedState('')).assertFalse()
      expect(RdpClientController.isConnectedState('Unknown')).assertFalse()
    })
  })
}
