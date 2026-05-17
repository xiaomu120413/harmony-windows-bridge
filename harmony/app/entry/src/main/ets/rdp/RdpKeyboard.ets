import {
  OH_KEYCODE_0,
  OH_KEYCODE_9,
  OH_KEYCODE_A,
  OH_KEYCODE_ALT_LEFT,
  OH_KEYCODE_ALT_RIGHT,
  OH_KEYCODE_APOSTROPHE,
  OH_KEYCODE_BACKSLASH,
  OH_KEYCODE_COMMA,
  OH_KEYCODE_CTRL_LEFT,
  OH_KEYCODE_CTRL_RIGHT,
  OH_KEYCODE_DEL,
  OH_KEYCODE_DPAD_DOWN,
  OH_KEYCODE_DPAD_LEFT,
  OH_KEYCODE_DPAD_RIGHT,
  OH_KEYCODE_DPAD_UP,
  OH_KEYCODE_ENTER,
  OH_KEYCODE_EQUALS,
  OH_KEYCODE_ESCAPE,
  OH_KEYCODE_F1,
  OH_KEYCODE_F12,
  OH_KEYCODE_FORWARD_DEL,
  OH_KEYCODE_GRAVE,
  OH_KEYCODE_INSERT,
  OH_KEYCODE_LEFT_BRACKET,
  OH_KEYCODE_META_LEFT,
  OH_KEYCODE_META_RIGHT,
  OH_KEYCODE_MINUS,
  OH_KEYCODE_MOVE_END,
  OH_KEYCODE_MOVE_HOME,
  OH_KEYCODE_NUMPAD_0,
  OH_KEYCODE_NUMPAD_9,
  OH_KEYCODE_NUMPAD_ADD,
  OH_KEYCODE_NUMPAD_DIVIDE,
  OH_KEYCODE_NUMPAD_DOT,
  OH_KEYCODE_NUMPAD_ENTER,
  OH_KEYCODE_NUMPAD_MULTIPLY,
  OH_KEYCODE_NUMPAD_SUBTRACT,
  OH_KEYCODE_PAGE_DOWN,
  OH_KEYCODE_PAGE_UP,
  OH_KEYCODE_PERIOD,
  OH_KEYCODE_RIGHT_BRACKET,
  OH_KEYCODE_SEMICOLON,
  OH_KEYCODE_SHIFT_LEFT,
  OH_KEYCODE_SHIFT_RIGHT,
  OH_KEYCODE_SLASH,
  OH_KEYCODE_SPACE,
  OH_KEYCODE_TAB,
  OH_KEYCODE_Z
} from './RdpConstants'

export class RdpKeyboard {
  static isPrintableKeyCode(keyCode: number): boolean {
    return (keyCode >= OH_KEYCODE_A && keyCode <= OH_KEYCODE_Z) ||
      (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9) ||
      (keyCode >= OH_KEYCODE_NUMPAD_0 && keyCode <= OH_KEYCODE_NUMPAD_9) ||
      keyCode === OH_KEYCODE_SPACE ||
      keyCode === OH_KEYCODE_GRAVE ||
      keyCode === OH_KEYCODE_MINUS ||
      keyCode === OH_KEYCODE_EQUALS ||
      keyCode === OH_KEYCODE_LEFT_BRACKET ||
      keyCode === OH_KEYCODE_RIGHT_BRACKET ||
      keyCode === OH_KEYCODE_BACKSLASH ||
      keyCode === OH_KEYCODE_SEMICOLON ||
      keyCode === OH_KEYCODE_APOSTROPHE ||
      keyCode === OH_KEYCODE_COMMA ||
      keyCode === OH_KEYCODE_PERIOD ||
      keyCode === OH_KEYCODE_SLASH ||
      keyCode === OH_KEYCODE_NUMPAD_DIVIDE ||
      keyCode === OH_KEYCODE_NUMPAD_MULTIPLY ||
      keyCode === OH_KEYCODE_NUMPAD_SUBTRACT ||
      keyCode === OH_KEYCODE_NUMPAD_ADD ||
      keyCode === OH_KEYCODE_NUMPAD_DOT
  }

  static shouldSendPrintableAsHardwareKey(keyCode: number): boolean {
    return (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9) ||
      (keyCode >= OH_KEYCODE_NUMPAD_0 && keyCode <= OH_KEYCODE_NUMPAD_9) ||
      keyCode === OH_KEYCODE_NUMPAD_DIVIDE ||
      keyCode === OH_KEYCODE_NUMPAD_MULTIPLY ||
      keyCode === OH_KEYCODE_NUMPAD_SUBTRACT ||
      keyCode === OH_KEYCODE_NUMPAD_ADD ||
      keyCode === OH_KEYCODE_NUMPAD_DOT
  }

  static isPlatformKeyCode(keyCode: number): boolean {
    if (RdpKeyboard.isPrintableKeyCode(keyCode) || RdpKeyboard.isFunctionKey(keyCode)) {
      return true
    }
    const keyCodes: number[] = [
      OH_KEYCODE_ESCAPE,
      OH_KEYCODE_TAB,
      OH_KEYCODE_ENTER,
      OH_KEYCODE_DEL,
      OH_KEYCODE_FORWARD_DEL,
      OH_KEYCODE_PAGE_UP,
      OH_KEYCODE_PAGE_DOWN,
      OH_KEYCODE_INSERT,
      OH_KEYCODE_DPAD_UP,
      OH_KEYCODE_DPAD_DOWN,
      OH_KEYCODE_DPAD_LEFT,
      OH_KEYCODE_DPAD_RIGHT,
      OH_KEYCODE_MOVE_HOME,
      OH_KEYCODE_MOVE_END,
      OH_KEYCODE_CTRL_LEFT,
      OH_KEYCODE_CTRL_RIGHT,
      OH_KEYCODE_ALT_LEFT,
      OH_KEYCODE_ALT_RIGHT,
      OH_KEYCODE_META_LEFT,
      OH_KEYCODE_META_RIGHT,
      OH_KEYCODE_SHIFT_LEFT,
      OH_KEYCODE_SHIFT_RIGHT,
      OH_KEYCODE_NUMPAD_ENTER
    ]
    return keyCodes.indexOf(keyCode) >= 0
  }

  static isDeleteKeyCode(keyCode: number): boolean {
    return keyCode === OH_KEYCODE_DEL || keyCode === OH_KEYCODE_FORWARD_DEL
  }

  static getHardwarePrintableText(event: KeyEvent): string {
    const unicode = RdpKeyboard.getEventUnicode(event)
    if (unicode > 0 && unicode <= 0xFFFF) {
      return String.fromCharCode(unicode)
    }
    return RdpKeyboard.getPrintableTextByKeyCode(event.keyCode)
  }

  static getPrintableTextByKeyCode(keyCode: number): string {
    if (keyCode >= OH_KEYCODE_A && keyCode <= OH_KEYCODE_Z) {
      return String.fromCharCode('a'.charCodeAt(0) + keyCode - OH_KEYCODE_A)
    }
    if (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9) {
      return `${keyCode - OH_KEYCODE_0}`
    }
    if (keyCode >= OH_KEYCODE_NUMPAD_0 && keyCode <= OH_KEYCODE_NUMPAD_9) {
      return `${keyCode - OH_KEYCODE_NUMPAD_0}`
    }
    const keyMap: Map<number, string> = new Map<number, string>([
      [OH_KEYCODE_NUMPAD_DIVIDE, '/'],
      [OH_KEYCODE_NUMPAD_MULTIPLY, '*'],
      [OH_KEYCODE_NUMPAD_SUBTRACT, '-'],
      [OH_KEYCODE_NUMPAD_ADD, '+'],
      [OH_KEYCODE_NUMPAD_DOT, '.']
    ])
    return keyMap.get(keyCode) ?? ''
  }

  static isModifierPressed(event: KeyEvent, modifier: string): boolean {
    const normalized = modifier.toLowerCase()
    const candidates: string[] = normalized === modifier ? [normalized] : [normalized, modifier]
    for (let i = 0; i < candidates.length; i++) {
      try {
        if (event.getModifierKeyState && event.getModifierKeyState([candidates[i]])) {
          return true
        }
      } catch (error) {
        continue
      }
    }
    return false
  }

  static isAnyModifierPressed(event: KeyEvent, modifiers: string[]): boolean {
    for (let i = 0; i < modifiers.length; i++) {
      if (RdpKeyboard.isModifierPressed(event, modifiers[i])) {
        return true
      }
    }
    return false
  }

  static isCtrlModifierPressed(event: KeyEvent): boolean {
    return RdpKeyboard.isAnyModifierPressed(event, ['ctrl'])
  }

  static isAltModifierPressed(event: KeyEvent): boolean {
    return RdpKeyboard.isAnyModifierPressed(event, ['alt'])
  }

  static isWinModifierPressed(event: KeyEvent): boolean {
    return RdpKeyboard.isAnyModifierPressed(event, ['meta', 'win'])
  }

  static isShiftModifierPressed(event: KeyEvent): boolean {
    return RdpKeyboard.isAnyModifierPressed(event, ['shift'])
  }

  static hasShortcutModifier(event: KeyEvent): boolean {
    return RdpKeyboard.isCtrlModifierPressed(event) || RdpKeyboard.isAltModifierPressed(event) ||
      RdpKeyboard.isWinModifierPressed(event) || RdpKeyboard.isShiftModifierPressed(event)
  }

  static isFunctionKey(keyCode: number): boolean {
    return keyCode >= OH_KEYCODE_F1 && keyCode <= OH_KEYCODE_F12
  }

  static getEventUnicode(event: KeyEvent): number {
    if (event.unicode && event.unicode > 0) {
      return event.unicode
    }
    if (event.keyText.length === 1) {
      return event.keyText.charCodeAt(0)
    }
    return 0
  }
}
