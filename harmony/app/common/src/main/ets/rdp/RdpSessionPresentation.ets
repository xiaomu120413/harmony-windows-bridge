export interface RdpSessionNotice {
  title: string
  subtitle: string
  remoteLoginWaiting: boolean
}

export class RdpSessionPresentation {
  static notice(state: string): RdpSessionNotice {
    if (state === 'Connecting') {
      return { title: '正在连接远程桌面', subtitle: '请稍候，正在建立安全连接', remoteLoginWaiting: false }
    }
    if (state === 'RemoteLoginWaiting') {
      return { title: '已连接，等待远端桌面', subtitle: '远端桌面登录阶段暂未收到新画面',
        remoteLoginWaiting: true }
    }
    if (state === 'Connected') {
      return { title: '已连接，正在打开远端桌面', subtitle: '等待远端桌面首帧', remoteLoginWaiting: false }
    }
    if (state === 'Resolving') {
      return { title: '正在解析主机', subtitle: '准备建立 RDP 连接', remoteLoginWaiting: false }
    }
    if (state === 'TCP connected') {
      return { title: '已连通主机', subtitle: '正在协商远程桌面会话', remoteLoginWaiting: false }
    }
    if (state === 'Negotiating') {
      return { title: '正在协商远程桌面', subtitle: '等待服务端返回图形能力', remoteLoginWaiting: false }
    }
    if (state === 'Authenticating') {
      return { title: '正在验证登录信息', subtitle: '即将进入远端桌面', remoteLoginWaiting: false }
    }
    return { title: '', subtitle: '', remoteLoginWaiting: false }
  }

  static failureMessage(message: string, fallback: string): string {
    const detail = message.trim()
    return detail.length === 0 ? fallback : `连接失败：${detail}`
  }
}
