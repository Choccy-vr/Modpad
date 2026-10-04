# Modpad
> An open-source, hot-swappable modular macro pad system with live browser-based key mapping.

Modpad combines hardware and software to create an adaptable modular macropad. Rather than locking users in a fixed, rigid layout, keyboard layout, or proprietary config software, Modpad allows users to magnetically hot-swap satellite modules into a main module and reconfigure key actions and layout all in real time.

BUILT IMAGE HERE
---
## Features
- **Dynamic Module Discovery:** Automatically identifies and polls attached satellite modules using i2c
- **Zero-Install Configurator:** Uses web serial to read active layouts, update bindings, and provide real-time input status all with a website.
- **Dynamic Remapping:** Keymaps are refreshed directly in the microcontroller's RAM; no need to restart every time.
- **Full HID & Consumer Control Support:** Emulates standard keyboard keystrokes, modifiers, and media controls (Volume, Play/Pause, Brightness).

## Modules
- **Macro:** The standard-sized macro pad. 6 completely remappable buttons.
- **Knobs:** 2 Rotary Encoders. Perfect for volume and brightness.
- More in the works

## Protocol
The web configurator and the main module communicate over USB serial (115200 baud) using newline-delimited JSON. Each message is a single JSON object ending in `\n`.

| Message | Direction | Description | Example |
|---|---|---|---|
| `get_layout` | Web -> Board | Requests the keymap of every module | `{"cmd":"get_layout"}` |
| `layout` | Board -> Web | Returns each module's keymap as a list of actions | `{"status":"layout","modules":{"Macro":[{"code":4,"mod":0,"consumer":false}, ...]}}` |
| `get_modules` | Web -> Board | Requests which modules are currently connected | `{"cmd":"get_modules"}` |
| `modules` | Board -> Web | Returns the connection state of each module | `{"status":"modules","modules":{"Macro":true,"Knobs":false}}` |
| `set_key` | Web -> Board | Rebinds one key or knob direction in RAM | `{"cmd":"set_key","module":"Macro","index":2,"code":4,"modifiers":0,"consumer":false}` |
| `ok` / `error` | Board -> Web | Reply to `set_key` | `{"status":"ok"}` / `{"status":"error","msg":"invalid_key_or_module"}` |
| `press` | Board -> Web | Sent whenever a button or knob is used | `{"status":"press","module":{"name":"Macro","index":2,"type":"button"}}` |
| `log` | Board -> Web | Debug message, so the serial stream stays pure JSON | `{"status":"log","msg":"..."}` |

### Fields
- `code`: HID keyboard usage ID, or a consumer usage ID when `consumer` is `true` (for example `0xE9` for Volume +).
- `modifiers` / `mod`: Modifier bitmask (`0x01` LCtrl, `0x02` LShift, `0x04` LAlt, `0x08` LGui, `0x10` RCtrl, `0x20` RShift, `0x40` RAlt, `0x80` RGui).
- `index`: Position in the module's keymap. It's the same index for `set_key` and `press`.

---

---
## Pictures
### Main Module
#### Schematic
<img width="1480" height="769" alt="image" src="https://github.com/user-attachments/assets/ff6c54dc-ecb7-4723-a2c1-465f6fe69b5c" />

#### PCB
<img width="1108" height="535" alt="image" src="https://github.com/user-attachments/assets/92d95ec4-9a26-41dc-8031-3432e05c2c1c" />

#### PCB 3D model
<img width="1386" height="754" alt="image" src="https://github.com/user-attachments/assets/31e63674-8cab-4d97-bba4-d254004dfc59" />
<img width="1386" height="754" alt="image" src="https://github.com/user-attachments/assets/1cbb065e-89eb-464c-9d3d-babad56df04a" />

#### CAD

### Macro Module
#### Schematic
<img width="1626" height="1117" alt="image" src="https://github.com/user-attachments/assets/87dd841d-4276-4908-b72a-ebd2d2796df8" />


#### PCB
<img width="815" height="837" alt="image" src="https://github.com/user-attachments/assets/7c0cb8bc-20c8-41c9-939b-9a793ba78de0" />


#### PCB 3D model
<img width="1280" height="1272" alt="image" src="https://github.com/user-attachments/assets/3204338a-ebb3-4baf-961a-efc1ec84bf32" />

<img width="1280" height="1272" alt="image" src="https://github.com/user-attachments/assets/011a89b1-1265-46c6-a942-9f26ca3a0bdf" />




#### CAD

  
<img width="1024" height="768" alt="Modpad_2026-Oct-04_01-32-06AM-000_CustomizedView5351986487" src="https://github.com/user-attachments/assets/721a257f-bdcb-43d1-b9a8-5ff951dfcf75" />


### Knobs Module
#### Schematic
<img width="1634" height="1128" alt="image" src="https://github.com/user-attachments/assets/c4f5a0a1-9899-41c8-83aa-4ea4c95c35f1" />

#### PCB
<img width="718" height="722" alt="image" src="https://github.com/user-attachments/assets/821f5bc7-42f1-4e30-8226-2fa138a35bbc" />


#### PCB 3D model
<img width="1267" height="1272" alt="image" src="https://github.com/user-attachments/assets/17e6505b-f3d4-4c3d-9db2-778bdc9508da" />
<img width="1267" height="1272" alt="image" src="https://github.com/user-attachments/assets/8c3bc560-1c0e-4731-8524-dbb1ce5b2e77" />


#### CAD
