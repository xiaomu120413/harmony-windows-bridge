export type ConnectionValidationIssue = 'hostRequired' | 'hostTooLong' | 'hostSpaces' |
  'hostProtocolOrPath' | 'hostFormat' | 'hostIpFormat' | 'portRequired' | 'portDigits' |
  'portRange' | 'usernameRequired' | 'passwordRequired'

export interface ConnectionValidationResult {
  ok: boolean
  issues: ConnectionValidationIssue[]
  host: string
  port: string
  username: string
  password: string
}

export class RdpConnectionValidator {
  static validate(hostInput: string, portInput: string, usernameInput: string,
    password: string): ConnectionValidationResult {
    const host = hostInput.trim()
    const port = portInput.trim()
    const username = usernameInput.trim()
    const issues: ConnectionValidationIssue[] = []
    const hostIssue = RdpConnectionValidator.validateHost(host)
    if (hostIssue !== null) {
      issues.push(hostIssue)
    }
    const portIssue = RdpConnectionValidator.validatePort(port)
    if (portIssue !== null) {
      issues.push(portIssue)
    }
    if (username.length === 0) {
      issues.push('usernameRequired')
    }
    if (password.trim().length === 0) {
      issues.push('passwordRequired')
    }
    return {
      ok: issues.length === 0,
      issues: issues,
      host: host,
      port: port,
      username: username,
      password: password
    }
  }

  private static validateHost(host: string): ConnectionValidationIssue | null {
    if (host.length === 0) {
      return 'hostRequired'
    }
    if (host.length > 253) {
      return 'hostTooLong'
    }
    if (/\s/.test(host)) {
      return 'hostSpaces'
    }
    if (host.indexOf('/') >= 0 || host.indexOf('\\') >= 0 || host.indexOf('://') >= 0) {
      return 'hostProtocolOrPath'
    }
    if (host.indexOf(':') >= 0) {
      return RdpConnectionValidator.isValidIpv6Host(host) ? null : 'hostFormat'
    }
    if (RdpConnectionValidator.isIpv4Candidate(host)) {
      return RdpConnectionValidator.isValidIpv4Host(host) ? null : 'hostIpFormat'
    }
    return RdpConnectionValidator.isValidDnsHost(host) ? null : 'hostFormat'
  }

  private static validatePort(port: string): ConnectionValidationIssue | null {
    if (port.length === 0) {
      return 'portRequired'
    }
    if (!/^[0-9]+$/.test(port)) {
      return 'portDigits'
    }
    const portNumber = Number(port)
    return portNumber < 1 || portNumber > 65535 ? 'portRange' : null
  }

  private static isIpv4Candidate(host: string): boolean {
    return /^[0-9.]+$/.test(host) && host.indexOf('.') >= 0
  }

  private static isValidIpv4Host(host: string): boolean {
    const parts = host.split('.')
    if (parts.length !== 4) {
      return false
    }
    for (const part of parts) {
      if (part.length === 0 || !/^[0-9]+$/.test(part)) {
        return false
      }
      const value = Number(part)
      if (value < 0 || value > 255 || Math.floor(value) !== value) {
        return false
      }
    }
    return true
  }

  private static isValidIpv6Host(host: string): boolean {
    return /^[0-9A-Fa-f:]+$/.test(host) && host.indexOf(':') >= 0 && host.split(':').length >= 3
  }

  private static isValidDnsHost(host: string): boolean {
    const labels = host.split('.')
    const labelPattern = /^[A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?$/
    for (const label of labels) {
      if (label.length === 0 || label.length > 63 || !labelPattern.test(label)) {
        return false
      }
    }
    return true
  }
}
