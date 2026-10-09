// Keymap editor for the clicked key or knob
import { type KeyboardEvent, useState } from 'react'
import { isModifierCode, usageFromEventCode } from '../modpad/keycodes'
import type { KeyAction } from '../modpad/protocol'
import {
  bindingLabel,
  hasModifier,
  keyLabel,
  knobIndex,
  MEDIA_CONTROLS,
  MODIFIER_BITS,
  type Selection,
} from './model'
import type { SaveStatus } from './useModpad'

interface KeyPanelProps {
  selection: Selection
  layout: Record<string, KeyAction[]>
  save: SaveStatus
  onSave: (module: string, index: number, action: KeyAction) => void
  onEdit: () => void
  onClose: () => void
}

export function KeyPanel({ selection, layout, save, onSave, onEdit, onClose }: KeyPanelProps) {
  const isKnob = selection.module === 'Knobs'
  // knobs have one binding per turn direction
  const [direction, setDirection] = useState(0)
  const index = isKnob ? knobIndex(selection.control, direction) : selection.control
  const action = layout[selection.module]?.[index]

  const title = isKnob ? `Knob ${selection.control + 1}` : `Key ${selection.control + 1}`

  return (
    <aside className="panel" aria-label="Edit binding">
      <header className="panel-head">
        <h2>
          {selection.module} · {title}
        </h2>
        <button className="btn btn-small" onClick={onClose}>
          Close
        </button>
      </header>

      {isKnob && (
        <div className="tabs" role="tablist">
          {['Turn right', 'Turn left'].map((label, d) => (
            <button
              key={d}
              role="tab"
              aria-selected={direction === d}
              onClick={() => {
                setDirection(d)
                onEdit()
              }}
            >
              {label}
            </button>
          ))}
        </div>
      )}

      {action ? (
        <>
          <p className="current">
            Current <strong>{bindingLabel(action)}</strong>
          </p>
          {/* keyed so the form resets when another key is picked or the pad reports a new binding */}
          <BindingForm
            key={`${selection.module}-${index}-${action.code}-${action.mod}-${action.consumer}`}
            action={action}
            save={save}
            onSave={(next) => onSave(selection.module, index, next)}
            onEdit={onEdit}
          />
        </>
      ) : (
        <p className="current">Reading binding from the pad…</p>
      )}
    </aside>
  )
}

interface BindingFormProps {
  action: KeyAction
  save: SaveStatus
  onSave: (action: KeyAction) => void
  onEdit: () => void
}

function BindingForm({ action, save, onSave, onEdit }: BindingFormProps) {
  const [kind, setKind] = useState<'key' | 'media'>(action.consumer ? 'media' : 'key')
  const [code, setCode] = useState(action.consumer ? 0 : action.code)
  const [mod, setMod] = useState(action.consumer ? 0 : action.mod)
  const [media, setMedia] = useState(action.consumer ? action.code : MEDIA_CONTROLS[0][0])
  const [hint, setHint] = useState<string | null>(null)

  const next: KeyAction =
    kind === 'key' ? { code, mod, consumer: false } : { code: media, mod: 0, consumer: true }
  const changed =
    next.code !== action.code || next.mod !== action.mod || next.consumer !== action.consumer
  const canSave = changed && next.code !== 0 && save.state !== 'saving'

  const edit = (fn: () => void) => {
    fn()
    setHint(null)
    onEdit()
  }

  const capture = (e: KeyboardEvent<HTMLInputElement>) => {
    // plain Tab still moves focus, and lone modifiers are read from the next key
    if (e.code === 'Tab' && !e.ctrlKey && !e.altKey && !e.metaKey && !e.shiftKey) return
    if (isModifierCode(e.code)) return
    e.preventDefault()

    const usage = usageFromEventCode(e.code)
    if (usage === undefined) {
      setHint(`${e.key} can't be sent by the pad.`)
      return
    }
    const held =
      (e.ctrlKey ? 0x01 : 0) | (e.shiftKey ? 0x02 : 0) | (e.altKey ? 0x04 : 0) | (e.metaKey ? 0x08 : 0)
    edit(() => {
      setCode(usage)
      // keep the ticked boxes unless the user held modifiers while pressing
      if (held) setMod(held)
    })
  }

  const toggleModifier = (bit: number, on: boolean) =>
    edit(() => setMod((m) => (on ? m | bit : m & ~(bit | (bit << 4)))))

  const reset = () =>
    edit(() => {
      setKind(action.consumer ? 'media' : 'key')
      setCode(action.consumer ? 0 : action.code)
      setMod(action.consumer ? 0 : action.mod)
      setMedia(action.consumer ? action.code : MEDIA_CONTROLS[0][0])
    })

  return (
    <form
      className="form"
      onSubmit={(e) => {
        e.preventDefault()
        if (canSave) onSave(next)
      }}
    >
      <fieldset>
        <legend>Type</legend>
        <label>
          <input type="radio" checked={kind === 'key'} onChange={() => edit(() => setKind('key'))} /> Key
        </label>
        <label>
          <input type="radio" checked={kind === 'media'} onChange={() => edit(() => setKind('media'))} />{' '}
          Media
        </label>
      </fieldset>

      {kind === 'key' ? (
        <>
          <label className="field">
            Key
            <input
              readOnly
              value={code ? keyLabel(code) : ''}
              placeholder="Click here, then press a key"
              onKeyDown={capture}
            />
            {hint && <span className="hint">{hint}</span>}
          </label>
          <fieldset>
            <legend>Modifiers</legend>
            {MODIFIER_BITS.map(([bit, name]) => (
              <label key={name}>
                <input
                  type="checkbox"
                  checked={hasModifier(mod, bit)}
                  onChange={(e) => toggleModifier(bit, e.target.checked)}
                />{' '}
                {name}
              </label>
            ))}
          </fieldset>
        </>
      ) : (
        <label className="field">
          Control
          <select value={media} onChange={(e) => edit(() => setMedia(Number(e.target.value)))}>
            {MEDIA_CONTROLS.map(([value, label]) => (
              <option key={value} value={value}>
                {label}
              </option>
            ))}
          </select>
        </label>
      )}

      <div className="actions">
        <button type="submit" className="btn btn-solid" disabled={!canSave}>
          {save.state === 'saving' ? 'Saving…' : 'Save'}
        </button>
        <button type="button" className="btn" onClick={reset} disabled={!changed}>
          Reset
        </button>
      </div>

      <p className="save-status" role="status">
        {save.state === 'saved' && 'Saved to the pad.'}
        {save.state === 'error' && `Couldn't save: ${save.msg}`}
      </p>
      <p className="note">Bindings live in the pad's memory and reset when it's unplugged.</p>
    </form>
  )
}
