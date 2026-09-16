import { describe, expect, it } from '@ohos/hypium'
import {
  RdpSessionStateCoordinator,
  RdpSessionStateSnapshot,
  SessionStateMessages,
  SessionStateConnectInput
} from '../main/ets/rdp/RdpSessionStateCoordinator'
import { RdpClientController, RdpClientCallbacks } from '../main/ets/rdp/RdpClientController'
import { NativeCommandResult, NativeConnectParams } from '../main/ets/rdp/NativeRdpGateway'
import { WindowsConnectionSaveInput } from '../main/ets/rdp/WindowsConnectionStore'
import { ConnectionValidationIssue } from '../main/ets/rdp/RdpConnectionValidator'

class FakeRdpClientController extends RdpClientController {
  private readonly registerResult: NativeCommandResult
  private readonly connectResult: NativeCommandResult
  private readonly connectedState: boolean
  capturedCallbacks: RdpClientCallbacks | null = null

  constructor(registerResult: NativeCommandResult, connectResult: NativeCommandResult,
    connectedState: boolean) {
    super()
    this.registerResult = registerResult
    this.connectResult = connectResult
    this.connectedState = connectedState
  }

  registerCallbacks(callbacks: RdpClientCallbacks): NativeCommandResult {
    this.capturedCallbacks = callbacks
    return this.registerResult
  }

  connect(_params: NativeConnectParams): NativeCommandResult {
    return this.connectResult
  }

  beginConnect(): void {}

  isConnected(): boolean {
    return this.connectedState
  }

  releaseAllInput(): NativeCommandResult {
    return { ok: true, state: '', message: '' }
  }
}

const defaultMessages: SessionStateMessages = {
  sessionConnectingTitle: '正在连接远程桌面',
  sessionConnectingSubtitle: '正在建立 RDP 会话',
  sessionConnectedTitle: '已连接，正在打开远端桌面',
  sessionConnectedSubtitle: '等待远端桌面首帧',
  sessionRemoteLoginWaitingTitle: '已连接，等待远端桌面',
  sessionRemoteLoginWaitingSubtitle: '远端桌面登录阶段暂未收到新画面',
  sessionResolvingTitle: '正在解析主机',
  sessionResolvingSubtitle: '准备建立 RDP 连接',
  sessionTcpConnectedTitle: '已连通主机',
  sessionTcpConnectedSubtitle: '正在协商远程桌面会话',
  sessionNegotiatingTitle: '正在协商远程桌面',
  sessionNegotiatingSubtitle: '等待服务端返回图形能力',
  sessionAuthenticatingTitle: '正在验证登录信息',
  sessionAuthenticatingSubtitle: '即将进入远端桌面',
  connectionSuccess: '连接成功',
  connectionConnecting: '正在连接...',
  connectionFailure: '连接失败',
  connectionFailureCredentials: '连接失败，请检查 Host、账号或密码',
  connectionFailureNoSession: '连接失败，未建立远程桌面会话',
  connectionFailureWithDetail: (detail: string): string => `连接失败：${detail}`,
  validationMessage: (issues: ConnectionValidationIssue[]): string => issues.join('；')
}

function okResult(state: string = 'Resolving', message: string = ''): NativeCommandResult {
  return { ok: true, state, message }
}

function failResult(message: string = 'connect failed'): NativeCommandResult {
  return { ok: false, state: 'Failed', message }
}

function validInput(): SessionStateConnectInput {
  return {
    host: '192.168.1.1',
    port: '3389',
    username: 'admin',
    password: 'pass',
    certPolicy: 'tofu',
    selectedProfileId: 'p1',
    rememberPassword: true
  }
}

function invalidInput(): SessionStateConnectInput {
  return {
    host: '',
    port: '3389',
    username: 'admin',
    password: 'pass',
    certPolicy: 'tofu',
    selectedProfileId: '',
    rememberPassword: false
  }
}

interface CoordinatorSpy {
  coordinator: RdpSessionStateCoordinator
  controller: FakeRdpClientController
  snapshots: RdpSessionStateSnapshot[]
  persistCalls: WindowsConnectionSaveInput[]
  releaseCalls: number[]
}

function createSpy(registerResult: NativeCommandResult = okResult('Idle'),
  connectResult: NativeCommandResult = okResult(),
  connectedState: boolean = false): CoordinatorSpy {
  const controller = new FakeRdpClientController(registerResult, connectResult, connectedState)
  const snapshots: RdpSessionStateSnapshot[] = []
  const persistCalls: WindowsConnectionSaveInput[] = []
  const releaseCalls: number[] = []
  const coordinator = new RdpSessionStateCoordinator(
    controller,
    defaultMessages,
    (): string => '/test',
    (input: WindowsConnectionSaveInput): void => {
      persistCalls.push(input)
    },
    (snapshot: RdpSessionStateSnapshot): void => {
      snapshots.push(snapshot)
    },
    (): void => {
      releaseCalls.push(1)
    }
  )
  return { coordinator, controller, snapshots, persistCalls, releaseCalls }
}

function lastSnapshot(spy: CoordinatorSpy): RdpSessionStateSnapshot {
  return spy.snapshots[spy.snapshots.length - 1]
}

export default function rdpSessionStateCoordinatorTest(): void {
  describe('RdpSessionStateCoordinator', (): void => {
    it('snapshotReflectsInitialState', 0, (): void => {
      const spy = createSpy()
      const snap = spy.coordinator.snapshot()
      expect(snap.sessionVisible).assertFalse()
      expect(snap.remoteLoginWaiting).assertFalse()
      expect(snap.sessionNoticeTitle).assertEqual('')
      expect(snap.sessionNoticeSubtitle).assertEqual('')
      expect(snap.connectionFeedbackText).assertEqual('')
      expect(snap.connectionFeedbackTone).assertEqual('neutral')
    })

    it('handleStateResolvingSetsResolvingNotice', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.handleState('Resolving', false)
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertFalse()
      expect(snap.remoteLoginWaiting).assertFalse()
      expect(snap.sessionNoticeTitle).assertEqual('正在解析主机')
      expect(snap.sessionNoticeSubtitle).assertEqual('准备建立 RDP 连接')
    })

    it('handleStateConnectedFirstConnectTriggersSuccessAndPersist', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.connect(validInput())
      spy.coordinator.handleState('Connected', false)
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertTrue()
      expect(snap.connectionFeedbackText).assertEqual('连接成功')
      expect(snap.connectionFeedbackTone).assertEqual('ok')
      expect(spy.persistCalls.length).assertEqual(1)
      expect(spy.persistCalls[0].host).assertEqual('192.168.1.1')
      expect(spy.persistCalls[0].profileId).assertEqual('p1')
    })

    it('handleStateRemoteLoginWaitingSetsWaitingNotice', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.handleState('Connected', false)
      spy.coordinator.handleState('RemoteLoginWaiting', true)
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertTrue()
      expect(snap.remoteLoginWaiting).assertTrue()
      expect(snap.sessionNoticeTitle).assertEqual('已连接，等待远端桌面')
      expect(snap.sessionNoticeSubtitle).assertEqual('远端桌面登录阶段暂未收到新画面')
    })

    it('handleStateRemoteDesktopReadyClearsNotice', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.handleState('Connected', false)
      spy.coordinator.handleState('RemoteDesktopReady', true)
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertTrue()
      expect(snap.remoteLoginWaiting).assertFalse()
      expect(snap.sessionNoticeTitle).assertEqual('')
      expect(snap.sessionNoticeSubtitle).assertEqual('')
    })

    it('handleStateFailedSetsCredentialsFeedbackAndClearsPending', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.connect(validInput())
      spy.coordinator.handleState('Failed', true)
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertFalse()
      expect(snap.connectionFeedbackText).assertEqual('连接失败，请检查 Host、账号或密码')
      expect(snap.connectionFeedbackTone).assertEqual('danger')
      expect(spy.releaseCalls.length).assertEqual(1)
    })

    it('handleStateDisconnectedWithPendingSetsNoSessionFeedback', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.connect(validInput())
      spy.coordinator.handleState('Disconnected', true)
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertFalse()
      expect(snap.connectionFeedbackText).assertEqual('连接失败，未建立远程桌面会话')
      expect(snap.connectionFeedbackTone).assertEqual('danger')
      expect(spy.releaseCalls.length).assertEqual(1)
    })

    it('handleStateIdleWithPendingSetsNoSessionFeedback', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.connect(validInput())
      spy.coordinator.handleState('Idle', false)
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertFalse()
      expect(snap.connectionFeedbackText).assertEqual('连接失败，未建立远程桌面会话')
      expect(snap.connectionFeedbackTone).assertEqual('danger')
      expect(spy.releaseCalls.length).assertEqual(0)
    })

    it('handleErrorSetsDangerFeedbackAndClearsSession', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.connect(validInput())
      spy.coordinator.handleError('native crash', true)
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertFalse()
      expect(snap.remoteLoginWaiting).assertFalse()
      expect(snap.sessionNoticeTitle).assertEqual('')
      expect(snap.sessionNoticeSubtitle).assertEqual('')
      expect(snap.connectionFeedbackText).assertEqual('连接失败：native crash')
      expect(snap.connectionFeedbackTone).assertEqual('danger')
      expect(spy.releaseCalls.length).assertEqual(1)
    })

    it('connectValidationFailureSetsWarningFeedback', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.connect(invalidInput())
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertFalse()
      expect(snap.connectionFeedbackTone).assertEqual('warning')
      expect(snap.connectionFeedbackText.length > 0).assertTrue()
      expect(spy.persistCalls.length).assertEqual(0)
    })

    it('connectSuccessSetsConnectingFeedbackAndSessionVisible', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.connect(validInput())
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertTrue()
      expect(snap.connectionFeedbackText).assertEqual('正在连接...')
      expect(snap.connectionFeedbackTone).assertEqual('info')
      expect(snap.sessionNoticeTitle).assertEqual('正在连接远程桌面')
      expect(snap.sessionNoticeSubtitle).assertEqual('正在建立 RDP 会话')
      expect(snap.remoteLoginWaiting).assertFalse()
    })

    it('handleConnectResultOkWithConnectedTriggersSuccessAndPersist', 0, (): void => {
      const spy = createSpy(okResult('Idle'), okResult('Resolving'), true)
      spy.coordinator.connect(validInput())
      spy.coordinator.handleConnectResult(okResult('Connected'))
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertTrue()
      expect(snap.connectionFeedbackText).assertEqual('连接成功')
      expect(snap.connectionFeedbackTone).assertEqual('ok')
      expect(spy.persistCalls.length).assertEqual(1)
    })

    it('handleConnectResultNotOkSetsDangerFeedbackAndClearsSession', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.connect(validInput())
      spy.coordinator.handleConnectResult(failResult('auth error'))
      const snap = lastSnapshot(spy)
      expect(snap.sessionVisible).assertFalse()
      expect(snap.connectionFeedbackText).assertEqual('连接失败：auth error')
      expect(snap.connectionFeedbackTone).assertEqual('danger')
    })

    it('registerIsIdempotent', 0, (): void => {
      const spy = createSpy(okResult('Idle'))
      const r1 = spy.coordinator.register()
      expect(r1.ok).assertTrue()
      expect(spy.controller.capturedCallbacks !== null).assertTrue()
      const r2 = spy.coordinator.register()
      expect(r2.ok).assertTrue()
      expect(r2.message).assertEqual('session state callbacks already registered')
    })

    it('registerReturnsFailureWhenNativeFails', 0, (): void => {
      const spy = createSpy(failResult('native unavailable'))
      const result = spy.coordinator.register()
      expect(result.ok).assertFalse()
    })

    it('clearConnectionFeedbackResetsFeedback', 0, (): void => {
      const spy = createSpy()
      spy.coordinator.setConnectionFeedback('some text', 'danger')
      spy.coordinator.clearConnectionFeedback()
      const snap = lastSnapshot(spy)
      expect(snap.connectionFeedbackText).assertEqual('')
      expect(snap.connectionFeedbackTone).assertEqual('neutral')
    })
  })
}
