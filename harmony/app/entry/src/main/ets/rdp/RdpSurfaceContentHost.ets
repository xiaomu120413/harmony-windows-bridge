import { NodeContent } from '@kit.ArkUI'
import { NativeRdpGateway } from './NativeRdpGateway'
import { RdpLogger } from './RdpLogger'

export class RdpSurfaceContentHost {
  attach(content: NodeContent): boolean {
    const result = NativeRdpGateway.attachXComponentContent(content)
    if (!result.ok) {
      RdpLogger.error(`XComponent content attach failed: ${result.message}`)
    }
    return result.ok
  }

  detach(): void {
    const result = NativeRdpGateway.detachXComponentContent()
    if (!result.ok) {
      RdpLogger.warn(`XComponent content detach failed: ${result.message}`)
    }
  }
}
