import deviceCapabilityPolicyTest from './DeviceCapabilityPolicy.test'
import displayOrientationPolicyTest from './DisplayOrientationPolicy.test'
import windowLayoutPolicyTest from './WindowLayoutPolicy.test'

export default function testsuite(): void {
  deviceCapabilityPolicyTest()
  displayOrientationPolicyTest()
  windowLayoutPolicyTest()
}
