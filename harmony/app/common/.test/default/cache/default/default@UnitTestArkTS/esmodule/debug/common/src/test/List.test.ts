import deviceCapabilityPolicyTest from "@normalized:N&&&common/src/test/DeviceCapabilityPolicy.test&";
import rdpConnectionValidatorTest from "@normalized:N&&&common/src/test/RdpConnectionValidator.test&";
import windowLayoutPolicyTest from "@normalized:N&&&common/src/test/WindowLayoutPolicy.test&";
export default function testsuite(): void {
    deviceCapabilityPolicyTest();
    rdpConnectionValidatorTest();
    windowLayoutPolicyTest();
}
