import { ConnectionValidationIssue } from '../../rdp/RdpConnectionValidator'
import { HomeText } from './HomeText'

export class HomeConnectionValidation {
  static message(issues: ConnectionValidationIssue[]): string {
    const messages: string[] = []
    for (const issue of issues) {
      messages.push(HomeConnectionValidation.issueMessage(issue))
    }
    return messages.join(HomeText.VALIDATION_SEPARATOR)
  }

  private static issueMessage(issue: ConnectionValidationIssue): string {
    if (issue === 'hostRequired') return HomeText.HOST_REQUIRED
    if (issue === 'hostTooLong') return HomeText.HOST_TOO_LONG
    if (issue === 'hostSpaces') return HomeText.HOST_NO_SPACES
    if (issue === 'hostProtocolOrPath') return HomeText.HOST_NO_PROTOCOL_OR_PATH
    if (issue === 'hostFormat') return HomeText.HOST_FORMAT_INVALID
    if (issue === 'hostIpFormat') return HomeText.HOST_IP_FORMAT_INVALID
    if (issue === 'portRequired') return HomeText.PORT_REQUIRED
    if (issue === 'portDigits') return HomeText.PORT_DIGITS_ONLY
    if (issue === 'portRange') return HomeText.PORT_RANGE_INVALID
    if (issue === 'usernameRequired') return HomeText.USERNAME_REQUIRED
    if (issue === 'passwordRequired') return HomeText.PASSWORD_REQUIRED
    return HomeText.CONNECTION_FAILURE
  }
}
