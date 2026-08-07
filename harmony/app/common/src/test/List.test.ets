import deviceCapabilityPolicyTest from './DeviceCapabilityPolicy.test'
import rdpConnectionValidatorTest from './RdpConnectionValidator.test'
import windowLayoutPolicyTest from './WindowLayoutPolicy.test'

export default function testsuite(): void {
  deviceCapabilityPolicyTest()
  rdpConnectionValidatorTest()
  windowLayoutPolicyTest()
}
