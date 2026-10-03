import type { ModpadCommand, ModpadMessage } from './protocol'

// Matches monitor_speed in platformio.ini, ignored by USB CDC but required by open()
const BAUD_RATE = 115200

export type ConnectionState = 'disconnected' | 'connecting' | 'connected'

export interface ModpadSerialEvents {
  onState?: (state: ConnectionState) => void
  // parsed JSON message from the firmware, log messages go to onLog instead
  onMessage?: (msg: ModpadMessage) => void
  // firmware debug messages, plus anything that isn't valid JSON
  onLog?: (line: string) => void
  // every raw line in either direction, for the debug console
  onRaw?: (dir: 'rx' | 'tx', line: string) => void
}

// Talks to the Modpad over Web Serial using the newline-delimited JSON protocol
export class ModpadSerial {
  private port: SerialPort | null = null
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null
  private readLoop: Promise<void> | null = null
  private encoder = new TextEncoder()
  private events: ModpadSerialEvents
  state: ConnectionState = 'disconnected'

  constructor(events: ModpadSerialEvents) {
    this.events = events
  }

  static isSupported(): boolean {
    return typeof navigator !== 'undefined' && !!navigator.serial
  }

  // Asks the user to pick a port, then opens it
  async connect(): Promise<void> {
    if (!navigator.serial) throw new Error('Web Serial is not supported')
    if (this.state !== 'disconnected') return

    this.setState('connecting')
    try {
      const port = await navigator.serial.requestPort()
      await port.open({ baudRate: BAUD_RATE })
      // TinyUSB CDC only sends once the host asserts DTR
      await port.setSignals({ dataTerminalReady: true, requestToSend: true })

      this.port = port
      this.writer = port.writable!.getWriter()
      this.reader = port.readable!.getReader()
      this.readLoop = this.runReadLoop(this.reader)
      this.setState('connected')
    } catch (err) {
      await this.cleanup()
      throw err
    }
  }

  async disconnect(): Promise<void> {
    await this.cleanup()
  }

  async send(command: ModpadCommand): Promise<void> {
    if (!this.writer) throw new Error('Not connected')
    const line = JSON.stringify(command)
    this.events.onRaw?.('tx', line)
    await this.writer.write(this.encoder.encode(line + '\n'))
  }

  getLayout() {
    return this.send({ cmd: 'get_layout' })
  }

  getModules() {
    return this.send({ cmd: 'get_modules' })
  }

  setKey(
    module: string,
    index: number,
    code: number,
    modifiers = 0,
    consumer = false,
  ) {
    return this.send({ cmd: 'set_key', module, index, code, modifiers, consumer })
  }

  private setState(state: ConnectionState) {
    this.state = state
    this.events.onState?.(state)
  }

  private async runReadLoop(reader: ReadableStreamDefaultReader<Uint8Array>) {
    const decoder = new TextDecoder()
    let buffer = ''
    try {
      for (;;) {
        const { value, done } = await reader.read()
        if (done) break
        buffer += decoder.decode(value, { stream: true })

        let newline: number
        while ((newline = buffer.indexOf('\n')) >= 0) {
          const line = buffer.slice(0, newline).replace(/\r$/, '')
          buffer = buffer.slice(newline + 1)
          if (line.trim()) this.handleLine(line)
        }
      }
    } catch (err) {
      // device unplugged or port lost
      this.events.onLog?.(`[serial] read error: ${String(err)}`)
    } finally {
      reader.releaseLock()
      if (this.reader === reader) {
        this.reader = null
        void this.cleanup()
      }
    }
  }

  private handleLine(line: string) {
    this.events.onRaw?.('rx', line)

    let msg: ModpadMessage
    try {
      msg = JSON.parse(line) as ModpadMessage
    } catch {
      // the firmware only sends JSON, so this is boot noise or a crash dump
      this.events.onLog?.(`[serial] non-JSON: ${line}`)
      return
    }

    if (msg.status === 'log') this.events.onLog?.(msg.msg)
    else this.events.onMessage?.(msg)
  }

  private async cleanup() {
    const { port, reader, writer, readLoop } = this
    this.port = null
    this.reader = null
    this.writer = null
    this.readLoop = null

    try {
      await reader?.cancel()
    } catch {
      /* already closed */
    }
    await readLoop
    try {
      writer?.releaseLock()
    } catch {
      /* already released */
    }
    try {
      await port?.close()
    } catch {
      /* already closed */
    }
    if (this.state !== 'disconnected') this.setState('disconnected')
  }
}
