import { describe, expect, it } from '@ohos/hypium'
import { common } from '@kit.AbilityKit'
import {
  RemoteControlCoordinator,
  XrdpServerDisplayMessages,
  RemoteControlSnapshot,
  XrdpServerStatus
} from '../main/ets/rdp/RemoteControlCoordinator'
import {
  RemoteControlPort,
  RemoteControlProcessParams,
  RemoteControlProcessResult
} from '../main/ets/rdp/RemoteControlPort'
import {
  RdpPermissionManager,
  RdpPermissionCallbackDelegate
} from '../main/ets/rdp/RdpPermissions'

class FakeRemoteControlPort implements RemoteControlPort {
  readonly available: boolean
  private readonly startResult: RemoteControlProcessResult
  private readonly diagnosticsResult: RemoteControlProcessResult

  constructor(available: boolean, startResult: RemoteControlProcessResult,
    diagnosticsResult: RemoteControlProcessResult) {
    this.available = available
    this.startResult = startResult
    this.diagnosticsResult = diagnosticsResult
  }

  start(_params: RemoteControlProcessParams): RemoteControlProcessResult {
    return this.startResult
  }

  diagnostics(): RemoteControlProcessResult {
    return this.diagnosticsResult
  }

  stop(_reason: string): RemoteControlProcessResult {
    return this.diagnosticsResult
  }
}

class FakePermissionManager extends RdpPermissionManager {
  private readonly screenRecordingGranted: boolean
  private readonly controlDeviceGranted: boolean
  private readonly ensureScreenRecordingResult: boolean
  private readonly requestScreenRecordingResult: boolean

  constructor(screenRecordingGranted: boolean, controlDeviceGranted: boolean,
    ensureScreenRecordingResult: boolean, requestScreenRecordingResult: boolean) {
    super(new RdpPermissionCallbackDelegate((): common.UIAbilityContext | null => null))
    this.screenRecordingGranted = screenRecordingGranted
    this.controlDeviceGranted = controlDeviceGranted
    this.ensureScreenRecordingResult = ensureScreenRecordingResult
    this.requestScreenRecordingResult = requestScreenRecordingResult
  }

  hasScreenRecordingPermission(): boolean {
    return this.screenRecordingGranted
  }

  ensureScreenRecordingPermission(_trigger: string): Promise<boolean> {
    return Promise.resolve(this.ensureScreenRecordingResult)
  }

  requestScreenRecordingPermission(_trigger: string): Promise<boolean> {
    return Promise.resolve(this.requestScreenRecordingResult)
  }

  hasControlDevicePermission(): boolean {
    return this.controlDeviceGranted
  }

  requestControlDevicePermission(_trigger: string): Promise<boolean> {
    return Promise.resolve(false)
  }
}

const defaultMessages: XrdpServerDisplayMessages = {
  active: 'Active session',
  listening: 'Listening on port',
  stopped: 'Server stopped',
  exited: 'Server exited',
  failed: 'Server failed',
  unavailable: 'Server unavailable'
}

function makeResult(state: string, ok: boolean = true, port: number = 0,
  message: string = ''): RemoteControlProcessResult {
  return { ok, state, message: message || `msg:${state}`, pid: 0, port, lastExitCode: 0 }
}

function makeStartResult(state: string = 'Listening', ok: boolean = true,
  port: number = 3389): RemoteControlProcessResult {
  return { ok, state, message: `started:${state}`, pid: 1, port, lastExitCode: 0 }
}

function createCoordinator(
  available: boolean,
  diagnosticsResult: RemoteControlProcessResult,
  onChange: (snapshot: RemoteControlSnapshot) => void,
  screenRecordingGranted: boolean = false,
  ensureScreenRecordingResult: boolean = false,
  requestScreenRecordingResult: boolean = false,
  startResult: RemoteControlProcessResult = makeStartResult()
): RemoteControlCoordinator {
  const port = new FakeRemoteControlPort(available, startResult, diagnosticsResult)
  const pm = (): RdpPermissionManager => new FakePermissionManager(
    screenRecordingGranted, false, ensureScreenRecordingResult, requestScreenRecordingResult
  )
  const appFilesDir = (): string => '/test'
  return new RemoteControlCoordinator(port, defaultMessages, pm, appFilesDir, onChange)
}

const noopOnChange = (_snapshot: RemoteControlSnapshot): void => {}

export default function remoteControlCoordinatorTest(): void {
  describe('RemoteControlCoordinator', (): void => {
    it('snapshotReflectsInitialState', 0, (): void => {
      const coordinator = createCoordinator(true, makeResult('Stopped'), noopOnChange)
      const snap = coordinator.snapshot()
      expect(snap.accessCode).assertEqual('000000')
      expect(snap.accessCodeGateEnabled).assertFalse()
      expect(snap.screenRecordingPermissionGranted).assertFalse()
      expect(snap.screenRecordingPermissionBusy).assertFalse()
      expect(snap.inputInjectionPermissionGranted).assertFalse()
      expect(snap.inputInjectionPermissionBusy).assertFalse()
      expect(snap.serverRunning).assertFalse()
      expect(snap.serverState).assertEqual('Stopped')
      expect(snap.serverPort).assertEqual(3390)
      expect(snap.serverMessage).assertEqual('')
      expect(snap.serverBusy).assertFalse()
    })

    it('toStatusDerivesRunningFromOkAndState', 0, (): void => {
      const c1 = createCoordinator(true, makeResult('Listening', true, 3389), noopOnChange)
      c1.refreshDiagnostics()
      expect(c1.snapshot().serverRunning).assertTrue()

      const c2 = createCoordinator(true, makeResult('ActiveSession', true, 3389), noopOnChange)
      c2.refreshDiagnostics()
      expect(c2.snapshot().serverRunning).assertTrue()

      const c3 = createCoordinator(true, makeResult('Stopped', true, 3389), noopOnChange)
      c3.refreshDiagnostics()
      expect(c3.snapshot().serverRunning).assertFalse()

      const c4 = createCoordinator(true, makeResult('Exited', true, 3389), noopOnChange)
      c4.refreshDiagnostics()
      expect(c4.snapshot().serverRunning).assertFalse()

      const c5 = createCoordinator(true, makeResult('Listening', false, 3389), noopOnChange)
      c5.refreshDiagnostics()
      expect(c5.snapshot().serverRunning).assertFalse()
    })

    it('toStatusFallsBackTo3390WhenPortIsNotPositive', 0, (): void => {
      const c1 = createCoordinator(true, makeResult('Listening', true, 0), noopOnChange)
      c1.refreshDiagnostics()
      expect(c1.snapshot().serverPort).assertEqual(3390)

      const c2 = createCoordinator(true, makeResult('Listening', true, -1), noopOnChange)
      c2.refreshDiagnostics()
      expect(c2.snapshot().serverPort).assertEqual(3390)

      const c3 = createCoordinator(true, makeResult('Listening', true, 3389), noopOnChange)
      c3.refreshDiagnostics()
      expect(c3.snapshot().serverPort).assertEqual(3389)
    })

    it('displayMessageMapsKnownStates', 0, (): void => {
      const c1 = createCoordinator(true, makeResult('ActiveSession'), noopOnChange)
      c1.refreshDiagnostics()
      expect(c1.snapshot().serverMessage).assertEqual('Active session')

      const c2 = createCoordinator(true, makeResult('Listening'), noopOnChange)
      c2.refreshDiagnostics()
      expect(c2.snapshot().serverMessage).assertEqual('Listening on port')

      const c3 = createCoordinator(true, makeResult('Stopped'), noopOnChange)
      c3.refreshDiagnostics()
      expect(c3.snapshot().serverMessage).assertEqual('Server stopped')

      const c4 = createCoordinator(true, makeResult('Exited'), noopOnChange)
      c4.refreshDiagnostics()
      expect(c4.snapshot().serverMessage).assertEqual('Server exited')

      const c5 = createCoordinator(true, makeResult('Unavailable'), noopOnChange)
      c5.refreshDiagnostics()
      expect(c5.snapshot().serverMessage).assertEqual('Server unavailable')

      const c6 = createCoordinator(true, makeResult('Failed'), noopOnChange)
      c6.refreshDiagnostics()
      expect(c6.snapshot().serverMessage).assertEqual('Server failed')
    })

    it('displayMessageFallsBackForResultMessage', 0, (): void => {
      const c = createCoordinator(true, makeResult('CustomState', true, 3389, 'custom message'), noopOnChange)
      c.refreshDiagnostics()
      expect(c.snapshot().serverMessage).assertEqual('custom message')
    })

    it('applyStatusUpdatesFieldsAndPublishes', 0, (): void => {
      let publishCount: number = 0
      const onChange = (_snapshot: RemoteControlSnapshot): void => {
        publishCount = publishCount + 1
      }
      const c = createCoordinator(true, makeResult('Listening', true, 3389), onChange)
      const before = publishCount
      c.refreshDiagnostics()
      expect(publishCount > before).assertTrue()
      const snap = c.snapshot()
      expect(snap.serverRunning).assertTrue()
      expect(snap.serverState).assertEqual('Listening')
      expect(snap.serverPort).assertEqual(3389)
    })

    it('refreshPermissionStateReturnsFalseWhenUnavailable', 0, (): void => {
      const c = createCoordinator(false, makeResult('Unavailable', false, 3390), noopOnChange, true)
      const result = c.refreshPermissionState()
      expect(result).assertFalse()
      expect(c.snapshot().screenRecordingPermissionGranted).assertFalse()
      expect(c.snapshot().screenRecordingPermissionBusy).assertFalse()
    })

    it('refreshPermissionStateReturnsHasPermissionWhenAvailable', 0, (): void => {
      const c1 = createCoordinator(true, makeResult('Stopped'), noopOnChange, true)
      const r1 = c1.refreshPermissionState()
      expect(r1).assertTrue()
      expect(c1.snapshot().screenRecordingPermissionGranted).assertTrue()

      const c2 = createCoordinator(true, makeResult('Stopped'), noopOnChange, false)
      const r2 = c2.refreshPermissionState()
      expect(r2).assertFalse()
      expect(c2.snapshot().screenRecordingPermissionGranted).assertFalse()
    })

    it('generateAccessCodeProducesSixDigits', 0, (): void => {
      const c = createCoordinator(false, makeResult('Stopped'), noopOnChange)
      const code = c.regenerateAccessCode()
      expect(code.length).assertEqual(6)
      const regex = /^[0-9]{6}$/
      expect(regex.test(code)).assertTrue()
      const code2 = c.regenerateAccessCode()
      expect(code2.length).assertEqual(6)
      expect(regex.test(code2)).assertTrue()
    })

    it('setAccessCodeGateIsIdempotentForSameValue', 0, (): void => {
      const c = createCoordinator(true, makeResult('Stopped'), noopOnChange, false, false, false)
      const code1 = c.setAccessCodeGate(true)
      const code2 = c.setAccessCodeGate(true)
      expect(code1).assertEqual(code2)
      expect(c.snapshot().accessCodeGateEnabled).assertTrue()
    })

    it('ensureStartedReturnsExistingPromiseWhenBusy', 0, (): void => {
      const c = createCoordinator(true, makeResult('Stopped'), noopOnChange, false, false, false)
      const p1 = c.ensureStarted('test1')
      p1.then((): void => {}, (): void => {})
      const p2 = c.ensureStarted('test2')
      expect(p1 === p2).assertTrue()
    })

    it('ensureStartedReturnsDiagnosticsWhenUnavailable', 0, (): void => {
      const c = createCoordinator(false, makeResult('Unavailable', false, 3390), noopOnChange)
      c.ensureStarted('test')
      expect(c.snapshot().serverState).assertEqual('Unavailable')
      expect(c.snapshot().serverRunning).assertFalse()
    })

    it('applyPermissionResultFallsBackToHasPermission', 0, (done: Function): void => {
      const c = createCoordinator(
        true,
        makeResult('Stopped', true, 0),
        noopOnChange,
        true,
        false,
        false,
        makeStartResult('Listening', true, 3389)
      )
      c.ensureStarted('test').then((status: XrdpServerStatus): void => {
        expect(status.state).assertEqual('Listening')
        done()
      }, (): void => {
        done()
      })
    })

    it('applyPermissionResultDeniesWhenBothGrantedAndHasPermissionAreFalse', 0, (done: Function): void => {
      const c = createCoordinator(
        true,
        makeResult('Stopped', true, 0),
        noopOnChange,
        false,
        false,
        false,
        makeStartResult('Listening', true, 3389)
      )
      c.ensureStarted('test').then((status: XrdpServerStatus): void => {
        expect(status.state).assertEqual('Stopped')
        done()
      }, (): void => {
        done()
      })
    })
  })
}
