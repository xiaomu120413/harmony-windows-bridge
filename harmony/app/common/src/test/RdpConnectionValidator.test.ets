import { describe, expect, it } from '@ohos/hypium'
import { RdpConnectionValidator } from '../main/ets/rdp/RdpConnectionValidator'

export default function rdpConnectionValidatorTest(): void {
  describe('RdpConnectionValidator', (): void => {
    it('normalizesValidConnection', 0, (): void => {
      const result = RdpConnectionValidator.validate(' server.example.com ', ' 3389 ', ' user ', 'secret')
      expect(result.ok).assertTrue()
      expect(result.host).assertEqual('server.example.com')
      expect(result.port).assertEqual('3389')
      expect(result.username).assertEqual('user')
      expect(result.issues.length).assertEqual(0)
    })

    it('reportsRequiredFieldsInStableOrder', 0, (): void => {
      const result = RdpConnectionValidator.validate('', '', '', '')
      expect(result.ok).assertFalse()
      expect(result.issues.join(',')).assertEqual(
        'hostRequired,portRequired,usernameRequired,passwordRequired')
    })

    it('acceptsIpv4AndIpv6Hosts', 0, (): void => {
      expect(RdpConnectionValidator.validate('192.168.1.8', '3389', 'u', 'p').ok).assertTrue()
      expect(RdpConnectionValidator.validate('2001:db8::1', '3389', 'u', 'p').ok).assertTrue()
    })

    it('rejectsInvalidHostAndPort', 0, (): void => {
      const result = RdpConnectionValidator.validate('https://host/path', '70000', 'u', 'p')
      expect(result.issues.join(',')).assertEqual('hostProtocolOrPath,portRange')
    })

    it('rejectsMalformedIpv4Candidate', 0, (): void => {
      const result = RdpConnectionValidator.validate('256.1.1.1', 'abc', 'u', 'p')
      expect(result.issues.join(',')).assertEqual('hostIpFormat,portDigits')
    })
  })
}
