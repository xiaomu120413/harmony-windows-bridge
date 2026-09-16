import deviceCapabilityPolicyTest from './DeviceCapabilityPolicy.test'
import rdpConnectionValidatorTest from './RdpConnectionValidator.test'
import rdpClientControllerTest from './RdpClientController.test'
import remoteControlCoordinatorTest from './RemoteControlCoordinator.test'
import rdpSessionStateCoordinatorTest from './RdpSessionStateCoordinator.test'
import windowLayoutPolicyTest from './WindowLayoutPolicy.test'

export default function testsuite(): void {
  deviceCapabilityPolicyTest()
  rdpConnectionValidatorTest()
  rdpClientControllerTest()
  remoteControlCoordinatorTest()
  rdpSessionStateCoordinatorTest()
  windowLayoutPolicyTest()
}
