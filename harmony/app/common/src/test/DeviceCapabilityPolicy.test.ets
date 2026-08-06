import { describe, expect, it } from '@ohos/hypium'
import {
  capabilitiesForDeviceType,
  RemoteControlServerCapability
} from '../main/ets/capability/DeviceCapabilityPolicy'

export default function deviceCapabilityPolicyTest(): void {
  describe('DeviceCapabilityPolicy', (): void => {
    it('enablesRemoteControlServerOn2in1', 0, (): void => {
      expect(capabilitiesForDeviceType('2in1').remoteControlServer)
        .assertEqual(RemoteControlServerCapability.AVAILABLE)
    })

    it('disablesRemoteControlServerOnTablet', 0, (): void => {
      expect(capabilitiesForDeviceType('tablet').remoteControlServer)
        .assertEqual(RemoteControlServerCapability.UNAVAILABLE)
    })

    it('disablesRemoteControlServerOnUnknownDevice', 0, (): void => {
      expect(capabilitiesForDeviceType('unknown').remoteControlServer)
        .assertEqual(RemoteControlServerCapability.UNAVAILABLE)
    })

    it('disablesRemoteControlServerForEmptyDeviceType', 0, (): void => {
      expect(capabilitiesForDeviceType('').remoteControlServer)
        .assertEqual(RemoteControlServerCapability.UNAVAILABLE)
    })
  })
}
