// Labels and shared types for the configurator UI
import { keyboardName, type KeyAction } from '../modpad/protocol'

// A clicked control: a Macro key index, or a Knobs encoder number
export interface Selection {
  module: string
  control: number
}

// Physical keys on the Macro module, the firmware keymap may cover fewer
export const MACRO_KEY_COUNT = 6
export const KNOB_COUNT = 2

// Knobs keymap is encoder * 2 + direction (0 = CW, 1 = CCW)
export function knobIndex(encoder: number, direction: number): number {
  return encoder * 2 + direction
}

export const MEDIA_CONTROLS: [number, string][] = [
  [0xe9, 'Volume up'],
  [0xea, 'Volume down'],
  [0xe2, 'Mute'],
  [0xcd, 'Play / pause'],
  [0xb5, 'Next track'],
  [0xb6, 'Previous track'],
  [0x6f, 'Brightness up'],
  [0x70, 'Brightness down'],
]

const MEDIA_SHORT: Record<number, string> = {
  0x6f: 'Bright +',
  0x70: 'Bright -',
  0xb5: 'Next',
  0xb6: 'Prev',
  0xcd: 'Play',
  0xe2: 'Mute',
  0xe9: 'Vol +',
  0xea: 'Vol -',
}

// Left-hand modifier bits, right-hand ones are the same shifted by 4
export const MODIFIER_BITS: [number, string][] = [
  [0x01, 'Ctrl'],
  [0x02, 'Shift'],
  [0x04, 'Alt'],
  [0x08, 'Gui'],
]

export function hasModifier(mod: number, bit: number): boolean {
  return (mod & (bit | (bit << 4))) !== 0
}

function hex(n: number): string {
  return `0x${n.toString(16).toUpperCase()}`
}

export function keyLabel(code: number): string {
  return keyboardName(code) ?? hex(code)
}

// Human label for a binding, e.g. "Vol +" or "Shift+Gui+4"
export function bindingLabel(action: KeyAction): string {
  if (action.consumer) return MEDIA_SHORT[action.code] ?? hex(action.code)
  if (action.code === 0) return 'None'
  const mods = MODIFIER_BITS.filter(([bit]) => hasModifier(action.mod, bit)).map(([, name]) => name)
  return [...mods, keyLabel(action.code)].join('+')
}
