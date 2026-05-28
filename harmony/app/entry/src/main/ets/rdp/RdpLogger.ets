import { hilog } from '@kit.PerformanceAnalysisKit'
import {
  HILOG_DOMAIN,
  HILOG_TAG
} from './RdpConstants'

export class RdpLogger {
  static debug(line: string): void {
    hilog.debug(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line)
  }

  static info(line: string): void {
    hilog.info(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line)
  }

  static warn(line: string): void {
    hilog.warn(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line)
  }

  static error(line: string): void {
    hilog.error(HILOG_DOMAIN, HILOG_TAG, '%{private}s', line)
  }
}
