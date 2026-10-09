// KeyboardEvent.code <-> HID keyboard usage id (HID_KEY_* in TinyUSB) and a display name

interface HidKey {
  code: string
  usage: number
  name: string
}

const KEYS: HidKey[] = []

for (let i = 0; i < 26; i++) {
  const letter = String.fromCharCode(65 + i)
  KEYS.push({ code: `Key${letter}`, usage: 0x04 + i, name: letter })
}
for (let i = 1; i <= 9; i++) {
  KEYS.push({ code: `Digit${i}`, usage: 0x1d + i, name: String(i) })
}
KEYS.push({ code: 'Digit0', usage: 0x27, name: '0' })
for (let i = 1; i <= 12; i++) {
  KEYS.push({ code: `F${i}`, usage: 0x39 + i, name: `F${i}` })
}
for (let i = 1; i <= 9; i++) {
  KEYS.push({ code: `Numpad${i}`, usage: 0x58 + i, name: `Num ${i}` })
}

KEYS.push(
  { code: 'Enter', usage: 0x28, name: 'Enter' },
  { code: 'Escape', usage: 0x29, name: 'Esc' },
  { code: 'Backspace', usage: 0x2a, name: 'Backspace' },
  { code: 'Tab', usage: 0x2b, name: 'Tab' },
  { code: 'Space', usage: 0x2c, name: 'Space' },
  { code: 'Minus', usage: 0x2d, name: '-' },
  { code: 'Equal', usage: 0x2e, name: '=' },
  { code: 'BracketLeft', usage: 0x2f, name: '[' },
  { code: 'BracketRight', usage: 0x30, name: ']' },
  { code: 'Backslash', usage: 0x31, name: '\\' },
  { code: 'Semicolon', usage: 0x33, name: ';' },
  { code: 'Quote', usage: 0x34, name: "'" },
  { code: 'Backquote', usage: 0x35, name: '`' },
  { code: 'Comma', usage: 0x36, name: ',' },
  { code: 'Period', usage: 0x37, name: '.' },
  { code: 'Slash', usage: 0x38, name: '/' },
  { code: 'CapsLock', usage: 0x39, name: 'Caps Lock' },
  { code: 'PrintScreen', usage: 0x46, name: 'Print Screen' },
  { code: 'ScrollLock', usage: 0x47, name: 'Scroll Lock' },
  { code: 'Pause', usage: 0x48, name: 'Pause' },
  { code: 'Insert', usage: 0x49, name: 'Insert' },
  { code: 'Home', usage: 0x4a, name: 'Home' },
  { code: 'PageUp', usage: 0x4b, name: 'Page Up' },
  { code: 'Delete', usage: 0x4c, name: 'Delete' },
  { code: 'End', usage: 0x4d, name: 'End' },
  { code: 'PageDown', usage: 0x4e, name: 'Page Down' },
  { code: 'ArrowRight', usage: 0x4f, name: 'Right' },
  { code: 'ArrowLeft', usage: 0x50, name: 'Left' },
  { code: 'ArrowDown', usage: 0x51, name: 'Down' },
  { code: 'ArrowUp', usage: 0x52, name: 'Up' },
  { code: 'NumLock', usage: 0x53, name: 'Num Lock' },
  { code: 'NumpadDivide', usage: 0x54, name: 'Num /' },
  { code: 'NumpadMultiply', usage: 0x55, name: 'Num *' },
  { code: 'NumpadSubtract', usage: 0x56, name: 'Num -' },
  { code: 'NumpadAdd', usage: 0x57, name: 'Num +' },
  { code: 'NumpadEnter', usage: 0x58, name: 'Num Enter' },
  { code: 'Numpad0', usage: 0x62, name: 'Num 0' },
  { code: 'NumpadDecimal', usage: 0x63, name: 'Num .' },
  { code: 'IntlBackslash', usage: 0x64, name: 'Intl \\' },
  { code: 'ContextMenu', usage: 0x65, name: 'Menu' },
)

const BY_CODE = new Map(KEYS.map((k) => [k.code, k.usage]))
const BY_USAGE = new Map(KEYS.map((k) => [k.usage, k.name]))

// HID usage for a browser key event code, undefined for keys the firmware can't send
export function usageFromEventCode(code: string): number | undefined {
  return BY_CODE.get(code)
}

export function keyName(usage: number): string | undefined {
  return BY_USAGE.get(usage)
}

// Modifier keys pressed on their own are read from the event flags instead
export function isModifierCode(code: string): boolean {
  return /^(Control|Shift|Alt|Meta|OS)(Left|Right)$/.test(code)
}
