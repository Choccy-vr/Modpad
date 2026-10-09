// Bare-bones test UI for ModpadSerial, not the final configurator
import { useEffect, useState } from 'react'
import { type ConnectionState, ModpadSerial } from './modpad/ModpadSerial'
import { describeAction, type KeyAction, type ModpadMessage } from './modpad/protocol'

interface LogLine {
  id: number
  time: string
  text: string
  // raw tx/rx line, only shown when the toggle is on
  raw: boolean
}

interface LastPress {
  module: string
  index: number
  type: string
}

const MAX_LOG = 500
let nextLogId = 0

function App() {
  const [state, setState] = useState<ConnectionState>('disconnected')
  const [error, setError] = useState<string | null>(null)
  const [modules, setModules] = useState<Record<string, boolean>>({})
  const [layout, setLayout] = useState<Record<string, KeyAction[]>>({})
  const [lastPress, setLastPress] = useState<LastPress | null>(null)
  const [log, setLog] = useState<LogLine[]>([])
  const [showRaw, setShowRaw] = useState(false)
  const [serial] = useState(() => {
    const addLog = (text: string, raw = false) => {
      const line = { id: nextLogId++, time: new Date().toLocaleTimeString(), text, raw }
      setLog((prev) => [line, ...prev].slice(0, MAX_LOG))
    }

    const serial: ModpadSerial = new ModpadSerial({
      onState: (s) => {
        setState(s)
        addLog(`[web] ${s}`)
        if (s === 'connected') {
          void serial.getModules()
          void serial.getLayout()
        }
      },
      onMessage: (msg: ModpadMessage) => {
        switch (msg.status) {
          case 'modules':
            setModules(msg.modules)
            break
          case 'layout':
            setLayout(msg.modules)
            break
          case 'press':
            setLastPress({
              module: msg.module.name,
              index: msg.module.index,
              type: msg.module.type,
            })
            break
          case 'ok':
            addLog('[web] set_key ok')
            void serial.getLayout()
            break
          case 'error':
            addLog(`[web] firmware error: ${msg.msg}`)
            break
        }
      },
      onLog: (line) => addLog(line),
      onRaw: (dir, line) => {
        addLog(`${dir === 'tx' ? '>>' : '<<'} ${line}`, true)
      },
    })
    return serial
  })

  useEffect(() => {
    return () => {
      void serial.disconnect()
    }
  }, [serial])

  const run = async (fn: (s: ModpadSerial) => Promise<void>) => {
    setError(null)
    try {
      await fn(serial)
    } catch (err) {
      setError(String(err))
    }
  }

  if (!ModpadSerial.isSupported()) {
    return (
      <main>
        <h1>Modpad test</h1>
        <p className="error">
          Web Serial is not available. Use Chrome / Edge / Chromium over http://localhost
          or https.
        </p>
      </main>
    )
  }

  const connected = state === 'connected'

  return (
    <main>
      <h1>Modpad test</h1>

      <section className="row">
        <span className={`dot ${state}`} /> <strong>{state}</strong>
        {connected ? (
          <button onClick={() => run((s) => s.disconnect())}>Disconnect</button>
        ) : (
          <button disabled={state === 'connecting'} onClick={() => run((s) => s.connect())}>
            Connect
          </button>
        )}
        <button disabled={!connected} onClick={() => run((s) => s.getModules())}>
          get_modules
        </button>
        <button disabled={!connected} onClick={() => run((s) => s.getLayout())}>
          get_layout
        </button>
      </section>
      {error && <p className="error">{error}</p>}

      <section>
        <h2>Modules</h2>
        {Object.keys(modules).length === 0 ? (
          <p className="muted">none reported</p>
        ) : (
          <ul className="modules">
            {Object.entries(modules).map(([name, active]) => (
              <li key={name}>
                <span className={`dot ${active ? 'connected' : 'disconnected'}`} /> {name}
              </li>
            ))}
          </ul>
        )}
      </section>

      <section>
        <h2>Last press</h2>
        <p>
          {lastPress
            ? `${lastPress.module} ${lastPress.type} #${lastPress.index}`
            : <span className="muted">nothing yet</span>}
        </p>
      </section>

      <section>
        <h2>Layout</h2>
        {Object.keys(layout).length === 0 ? (
          <p className="muted">not loaded</p>
        ) : (
          Object.entries(layout).map(([name, actions]) => (
            <div key={name}>
              <h3>
                {name} {modules[name] ? '' : <span className="muted">(not connected)</span>}
              </h3>
              <table>
                <thead>
                  <tr>
                    <th>#</th>
                    <th>current</th>
                    <th>code</th>
                    <th>mod</th>
                    <th>consumer</th>
                    <th />
                  </tr>
                </thead>
                <tbody>
                  {actions.map((action, index) => (
                    <KeyRow
                      // remount when the firmware reports a new binding
                      key={`${index}-${action.code}-${action.mod}-${action.consumer}`}
                      index={index}
                      action={action}
                      pressed={lastPress?.module === name && lastPress.index === index}
                      disabled={!connected}
                      onSave={(code, mod, consumer) =>
                        run((s) => s.setKey(name, index, code, mod, consumer))
                      }
                    />
                  ))}
                </tbody>
              </table>
            </div>
          ))
        )}
      </section>

      <section>
        <h2>
          Log{' '}
          <label className="muted">
            <input
              type="checkbox"
              checked={showRaw}
              onChange={(e) => setShowRaw(e.target.checked)}
            />{' '}
            raw tx/rx
          </label>{' '}
          <button onClick={() => setLog([])}>clear</button>
        </h2>
        <pre className="log">
          {log
            .filter((l) => showRaw || !l.raw)
            .map((l) => `${l.time}  ${l.text}`)
            .join('\n')}
        </pre>
      </section>
    </main>
  )
}

interface KeyRowProps {
  index: number
  action: KeyAction
  pressed: boolean
  disabled: boolean
  onSave: (code: number, mod: number, consumer: boolean) => void
}

// Accepts decimal or 0x-prefixed hex
function parseNumber(value: string): number {
  return Number(value.trim())
}

function KeyRow({ index, action, pressed, disabled, onSave }: KeyRowProps) {
  const [code, setCode] = useState(`0x${action.code.toString(16)}`)
  const [mod, setMod] = useState(`0x${action.mod.toString(16)}`)
  const [consumer, setConsumer] = useState(action.consumer)

  const codeNum = parseNumber(code)
  const modNum = parseNumber(mod)
  const valid =
    Number.isInteger(codeNum) && codeNum >= 0 && codeNum <= 0xffff &&
    Number.isInteger(modNum) && modNum >= 0 && modNum <= 0xff

  return (
    <tr className={pressed ? 'pressed' : undefined}>
      <td>{index}</td>
      <td>{describeAction(action)}</td>
      <td>
        <input value={code} onChange={(e) => setCode(e.target.value)} size={7} />
      </td>
      <td>
        <input value={mod} onChange={(e) => setMod(e.target.value)} size={5} />
      </td>
      <td>
        <input
          type="checkbox"
          checked={consumer}
          onChange={(e) => setConsumer(e.target.checked)}
        />
      </td>
      <td>
        <button disabled={disabled || !valid} onClick={() => onSave(codeNum, modNum, consumer)}>
          set
        </button>
      </td>
    </tr>
  )
}

export default App
