// Empty grid, Main module outline once connected, one outline per attached module
import type { KeyAction } from '../modpad/protocol'
import { bindingLabel, KNOB_COUNT, knobIndex, MACRO_KEY_COUNT, type Selection } from './model'
import type { Press } from './useModpad'

interface CanvasProps {
  supported: boolean
  connected: boolean
  connecting: boolean
  attached: string[]
  layout: Record<string, KeyAction[]>
  selected: Selection | null
  press: Press | null
  screen: string
  onSelect: (selection: Selection) => void
  onConnect: () => void
}

// allow wrapping after each + in chords
function wrapLabel(label: string): string {
  return label.replaceAll('+', '+​')
}

export function Canvas({
  supported,
  connected,
  connecting,
  attached,
  layout,
  selected,
  press,
  screen,
  onSelect,
  onConnect,
}: CanvasProps) {
  const isSelected = (module: string, control: number) =>
    selected?.module === module && selected.control === control

  // a knob flashes for either turn direction
  const isPressed = (module: string, control: number) => {
    if (!press || press.module !== module) return false
    return press.type === 'encoder' ? Math.floor(press.index / 2) === control : press.index === control
  }

  // remounting on each press restarts the flash animation
  const flashKey = (module: string, control: number) =>
    `${control}-${isPressed(module, control) ? press?.id : ''}`

  if (!supported) {
    return (
      <section className="canvas canvas-empty" aria-label="Modules">
        <p>This browser can't talk to the pad.</p>
        <p>Open this page in Chrome or Edge on a computer.</p>
      </section>
    )
  }

  if (!connected) {
    return (
      <section className="canvas canvas-empty" aria-label="Modules">
        <p>No Modpad connected</p>
        <button className="btn" onClick={onConnect} disabled={connecting}>
          {connecting ? 'Connecting…' : 'Connect'}
        </button>
      </section>
    )
  }

  return (
    <section className="canvas" aria-label="Modules">
      <div className="module-slot">
        <div className="module module-main">
          <div className="screen">{screen}</div>
        </div>
        <p className="module-name">Main</p>
      </div>

      {attached.length === 0 ? (
        <p className="canvas-hint">Snap a module onto the Main module</p>
      ) : (
        <div className="satellites">
          {attached.map((name) => (
            <div key={name} className="module-slot">
              {name === 'Macro' ? (
                <div className="module module-macro">
                  {Array.from({ length: MACRO_KEY_COUNT }, (_, i) => {
                    const action = layout.Macro?.[i]
                    return (
                      <button
                        key={flashKey(name, i)}
                        className={`key${isPressed(name, i) ? ' flash' : ''}`}
                        aria-pressed={isSelected(name, i)}
                        aria-label={
                          action
                            ? `Key ${i + 1}: ${bindingLabel(action)}`
                            : `Key ${i + 1}, not in the firmware keymap`
                        }
                        disabled={!action}
                        onClick={() => onSelect({ module: name, control: i })}
                      >
                        {action && wrapLabel(bindingLabel(action))}
                      </button>
                    )
                  })}
                </div>
              ) : name === 'Knobs' ? (
                <div className="module module-knobs">
                  {Array.from({ length: KNOB_COUNT }, (_, encoder) => (
                    <button
                      key={flashKey(name, encoder)}
                      className={`knob${isPressed(name, encoder) ? ' flash' : ''}`}
                      aria-pressed={isSelected(name, encoder)}
                      aria-label={`Knob ${encoder + 1}`}
                      disabled={!layout.Knobs?.[knobIndex(encoder, 0)]}
                      onClick={() => onSelect({ module: name, control: encoder })}
                    >
                      {encoder + 1}
                    </button>
                  ))}
                </div>
              ) : (
                // a module the configurator can't draw yet, e.g. Slider
                <div className="module module-unknown" />
              )}
              <p className="module-name">{name}</p>
            </div>
          ))}
        </div>
      )}
    </section>
  )
}
