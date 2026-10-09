// Live connection to the pad: modules, keymaps, presses and saving bindings
import { useEffect, useState } from 'react'
import { type ConnectionState, ModpadSerial } from '../modpad/ModpadSerial'
import type { KeyAction, ModpadMessage } from '../modpad/protocol'

export interface Press {
  module: string
  index: number
  type: 'button' | 'encoder'
  // bumps on every press so repeated presses of one key still flash
  id: number
}

export type SaveStatus =
  | { state: 'idle' }
  | { state: 'saving' }
  | { state: 'saved' }
  | { state: 'error'; msg: string }

// set_key replies carry no id, give up waiting after this long
const SAVE_TIMEOUT_MS = 2000
const FLASH_MS = 250

const ERROR_TEXT: Record<string, string> = {
  invalid_key_or_module: 'The pad has no binding at that position.',
}

let nextPressId = 0

// Keeps known modules in the order they were attached, new ones go last
function mergeAttached(prev: string[], reported: Record<string, boolean>): string[] {
  const on = Object.keys(reported).filter((name) => reported[name])
  const kept = prev.filter((name) => on.includes(name))
  return [...kept, ...on.filter((name) => !kept.includes(name))]
}

export function useModpad() {
  const [state, setState] = useState<ConnectionState>('disconnected')
  const [error, setError] = useState<string | null>(null)
  const [attached, setAttached] = useState<string[]>([])
  const [layout, setLayout] = useState<Record<string, KeyAction[]>>({})
  // press flashes briefly, lastPress stays for the Main screen
  const [press, setPress] = useState<Press | null>(null)
  const [lastPress, setLastPress] = useState<Press | null>(null)
  const [save, setSave] = useState<SaveStatus>({ state: 'idle' })

  // timers live in this closure next to the serial instance, only event handlers touch them
  const [{ serial, startSave, finishSave, stopTimers }] = useState(() => {
    const timers = { save: 0, flash: 0 }

    const finishSave = (status: SaveStatus) => {
      window.clearTimeout(timers.save)
      setSave((current) => (current.state === 'saving' ? status : current))
    }

    const startSave = () => {
      window.clearTimeout(timers.save)
      setSave({ state: 'saving' })
      timers.save = window.setTimeout(() => {
        finishSave({ state: 'error', msg: 'No reply from the pad.' })
      }, SAVE_TIMEOUT_MS)
    }

    const stopTimers = () => {
      window.clearTimeout(timers.save)
      window.clearTimeout(timers.flash)
    }

    const serial: ModpadSerial = new ModpadSerial({
      onState: (s) => {
        setState(s)
        if (s === 'connected') {
          void serial.getModules()
          void serial.getLayout()
        }
        if (s === 'disconnected') {
          setAttached([])
          setLayout({})
          setPress(null)
          setLastPress(null)
          finishSave({ state: 'idle' })
        }
      },
      onMessage: (msg: ModpadMessage) => {
        switch (msg.status) {
          case 'modules':
            setAttached((prev) => mergeAttached(prev, msg.modules))
            break
          case 'layout':
            setLayout(msg.modules)
            break
          case 'press': {
            window.clearTimeout(timers.flash)
            const { name, index, type } = msg.module
            const press = { module: name, index, type, id: nextPressId++ }
            setPress(press)
            setLastPress(press)
            timers.flash = window.setTimeout(() => setPress(null), FLASH_MS)
            break
          }
          case 'ok':
            finishSave({ state: 'saved' })
            // the firmware doesn't echo the binding, read it back
            void serial.getLayout()
            break
          case 'error':
            finishSave({ state: 'error', msg: ERROR_TEXT[msg.msg] ?? msg.msg })
            break
        }
      },
    })
    return { serial, startSave, finishSave, stopTimers }
  })

  useEffect(() => {
    return () => {
      stopTimers()
      void serial.disconnect()
    }
  }, [serial, stopTimers])

  const connect = async () => {
    setError(null)
    try {
      await serial.connect()
    } catch (err) {
      // closing the port picker isn't an error
      if (err instanceof DOMException && err.name === 'NotFoundError') return
      setError(err instanceof Error ? err.message : String(err))
    }
  }

  const disconnect = () => serial.disconnect()

  const refresh = async () => {
    await serial.getModules()
    await serial.getLayout()
  }

  const setKey = async (module: string, index: number, action: KeyAction) => {
    startSave()
    try {
      await serial.setKey(module, index, action.code, action.mod, action.consumer)
    } catch (err) {
      finishSave({ state: 'error', msg: err instanceof Error ? err.message : String(err) })
    }
  }

  const resetSave = () => setSave({ state: 'idle' })

  return {
    supported: ModpadSerial.isSupported(),
    state,
    error,
    attached,
    layout,
    press,
    lastPress,
    save,
    connect,
    disconnect,
    refresh,
    setKey,
    resetSave,
  }
}
