// Message shapes, mirrors ModPad-Firmware-Main/src/Configurator/Configurator.cpp
import { keyName } from './keycodes'

// One keymap entry as sent by sendAllKeymapsJson()
export interface KeyAction {
  code: number
  mod: number
  consumer: boolean
}

export type ModpadMessage =
  | { status: 'layout'; modules: Record<string, KeyAction[]> }
  | { status: 'modules'; modules: Record<string, boolean> }
  | {
      status: 'press'
      module: { name: string; index: number; type: 'button' | 'encoder' }
    }
  | { status: 'ok' }
  | { status: 'error'; msg: string }
  | { status: 'log'; msg: string }

// Commands handled by processConfigCommand()
export type ModpadCommand =
  | { cmd: 'get_layout' }
  | { cmd: 'get_modules' }
  | {
      cmd: 'set_key'
      module: string
      index: number
      code: number
      modifiers?: number
      consumer?: boolean
    }

// Modifier bits of the HID keyboard report
export const MODIFIERS = [
  { bit: 0x01, name: 'LCtrl' },
  { bit: 0x02, name: 'LShift' },
  { bit: 0x04, name: 'LAlt' },
  { bit: 0x08, name: 'LGui' },
  { bit: 0x10, name: 'RCtrl' },
  { bit: 0x20, name: 'RShift' },
  { bit: 0x40, name: 'RAlt' },
  { bit: 0x80, name: 'RGui' },
]

// Consumer usages the firmware keymaps currently use (HID_USAGE_CONSUMER_*)
const CONSUMER_NAMES: Record<number, string> = {
  0x6f: 'Brightness +',
  0x70: 'Brightness -',
  0xb5: 'Next Track',
  0xb6: 'Prev Track',
  0xcd: 'Play/Pause',
  0xe2: 'Mute',
  0xe9: 'Volume +',
  0xea: 'Volume -',
}

// Keyboard usage id -> label (HID_KEY_*)
export function keyboardName(code: number): string | undefined {
  return keyName(code)
}

export function describeAction(action: KeyAction): string {
  const hex = `0x${action.code.toString(16).padStart(2, '0')}`
  const name = action.consumer
    ? CONSUMER_NAMES[action.code]
    : keyboardName(action.code)
  const mods = MODIFIERS.filter((m) => action.mod & m.bit).map((m) => m.name)
  const label = [...mods, name ?? hex].join('+')
  return `${action.consumer ? 'consumer' : 'key'} ${label}`
}
