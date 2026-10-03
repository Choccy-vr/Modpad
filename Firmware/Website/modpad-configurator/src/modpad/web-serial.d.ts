// Minimal Web Serial API typings (Chromium only), just what ModpadSerial uses

interface SerialPortInfo {
  usbVendorId?: number
  usbProductId?: number
}

interface SerialOptions {
  baudRate: number
}

interface SerialOutputSignals {
  dataTerminalReady?: boolean
  requestToSend?: boolean
}

interface SerialPort extends EventTarget {
  readonly readable: ReadableStream<Uint8Array> | null
  readonly writable: WritableStream<Uint8Array> | null
  open(options: SerialOptions): Promise<void>
  close(): Promise<void>
  getInfo(): SerialPortInfo
  setSignals(signals: SerialOutputSignals): Promise<void>
}

interface SerialPortRequestOptions {
  filters?: { usbVendorId?: number; usbProductId?: number }[]
}

interface Serial extends EventTarget {
  requestPort(options?: SerialPortRequestOptions): Promise<SerialPort>
  getPorts(): Promise<SerialPort[]>
}

interface Navigator {
  readonly serial?: Serial
}
