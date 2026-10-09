// Modpad configurator, talks to the Main module over Web Serial
import { useState } from 'react'
import './app.css'
import { Canvas } from './configurator/Canvas'
import { KeyPanel } from './configurator/KeyPanel'
import type { Selection } from './configurator/model'
import { type Press, useModpad } from './configurator/useModpad'

// Mirrors what the firmware puts on the OLED for a press
function pressText(press: Press | null): string {
  if (!press) return ''
  if (press.type === 'encoder') {
    return `${press.module} ${Math.floor(press.index / 2) + 1} ${press.index % 2 === 0 ? 'CW' : 'CCW'}`
  }
  return `${press.module} ${press.index + 1}`
}

function App() {
  const pad = useModpad()
  const [selected, setSelected] = useState<Selection | null>(null)
  // last thing shown in the panel, kept so it doesn't go blank while sliding out
  const [shown, setShown] = useState<Selection | null>(null)

  const connected = pad.state === 'connected'
  // close the panel if its module was unplugged
  const active = connected && selected && pad.attached.includes(selected.module) ? selected : null

  const select = (selection: Selection) => {
    setSelected(selection)
    setShown(selection)
    pad.resetSave()
  }

  return (
    <div className="app">
      <header className="topbar">
        <h1>Modpad</h1>
        <div className="status">
          {pad.error && <span className="error">{pad.error}</span>}
          <span className={`dot${connected ? ' dot-on' : ''}`} />
          {connected ? 'Connected' : pad.state === 'connecting' ? 'Connecting…' : 'Not connected'}
          {connected && (
            <button className="btn btn-small" onClick={() => void pad.refresh()}>
              Refresh
            </button>
          )}
          {pad.supported && (
            <button
              className="btn btn-small"
              disabled={pad.state === 'connecting'}
              onClick={() => void (connected ? pad.disconnect() : pad.connect())}
            >
              {connected ? 'Disconnect' : 'Connect'}
            </button>
          )}
        </div>
      </header>

      <main className={`workspace${active ? ' workspace-open' : ''}`}>
        <Canvas
          supported={pad.supported}
          connected={connected}
          connecting={pad.state === 'connecting'}
          attached={pad.attached}
          layout={pad.layout}
          selected={active}
          press={pad.press}
          screen={pressText(pad.lastPress)}
          onSelect={select}
          onConnect={() => void pad.connect()}
        />
        <div className="panel-wrap" inert={!active}>
          {shown && (
            <KeyPanel
              // remount so the knob direction tab resets
              key={`${shown.module}-${shown.control}`}
              selection={shown}
              layout={pad.layout}
              save={pad.save}
              onSave={(module, index, action) => void pad.setKey(module, index, action)}
              onEdit={pad.resetSave}
              onClose={() => setSelected(null)}
            />
          )}
        </div>
      </main>
    </div>
  )
}

export default App
