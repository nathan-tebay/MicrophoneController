# MicrophoneController

A 2-button USB HID microphone controller built on the **nullbits Bit-C PRO (ATmega32U4)** microcontroller. Each button supports tap-to-toggle and hold-for-push-to-talk (PTT). A mode switch selects between keyboard and gamepad HID output modes. Dual LEDs reflect the active state of each microphone button.

## Features

- **Dual-mode HID output**: Switch between keyboard (Scroll Lock / Pause keys) and gamepad (buttons 8–9) modes
- **Tap-to-toggle**: Press once to activate a microphone latch that persists until toggled off
- **Push-to-talk (PTT)**: Hold a button to send a signal only while physically held; releases on button release
- **LED indicators**: Each button has an LED that reflects its active state
- **Serial debug output**: Status messages at 9600 baud

## Hardware

### Board

- **MCU**: ATmega32U4 (Arduino Leonardo-compatible)
- **Board**: nullbits Bit-C PRO

### Wiring

| Signal | Pin | Notes |
|--------|-----|-------|
| Button 1 | D2 | Connect to GND; internal pull-up enabled |
| Button 2 | D3 | Connect to GND; internal pull-up enabled |
| LED 1 | D4 | Connect through ~220 Ω resistor to GND |
| LED 2 | D5 | Connect through ~220 Ω resistor to GND |
| Mode switch | D15 | LOW (to GND) = keyboard mode; HIGH/floating = gamepad mode |

## Software requirements

### Toolchain
- **Arduino IDE** (latest) or `arduino-cli`
- **Board package**: Arduino AVR (built-in)

### Libraries

| Library | Source |
|---------|--------|
| `Keyboard` | Built-in (Arduino AVR core) |
| `HID` | Built-in (Arduino AVR core) |

Gamepad output uses the AVR core's `HIDSubDescriptor` directly — no third-party joystick library needed.

## Build & upload

### Using the build script (recommended)

```bash
./compile-upload.sh          # Compile and upload
./compile-upload.sh compile  # Compile only
./compile-upload.sh upload   # Upload only
```

The script:
- Auto-installs `dfu-programmer` if missing (apt or dnf)
- Compiles with custom USB identifiers (VID `0x1209`, PID `0x0001`)
- Uses `dfu-programmer` for flashing: erase → flash → reset
- Stores build artifacts in `.build/`

### Using Arduino IDE

1. Open **MicrophoneController.ino**
2. Select **Tools → Board → Arduino AVR Boards → Arduino Leonardo**
3. Compile: `Ctrl+R`
4. Upload: `Ctrl+U`

## Button behavior

### Keyboard mode (Mode pin LOW)

| Action | Behavior |
|--------|----------|
| **Tap** | Toggle microphone latch state; LED tracks latch |
| **Hold** (≥50 ms) | Push-to-talk: signal active only while held; clears on release |

**Default key assignments**:
- Button 1: `Scroll Lock` (0xCF)
- Button 2: `Pause / Break` (0xE1)

### Gamepad mode (Mode pin HIGH)

| Action | Behavior |
|--------|----------|
| **Tap** | Toggle joystick button latch state; LED tracks latch |
| **Hold** (≥50 ms) | Push-to-talk: joystick button active only while held; clears on release |

**Button assignments**:
- Button 1: Joystick button 8
- Button 2: Joystick button 9

## Remapping keys

Keys are hardcoded compile-time constants. Edit `MicrophoneController.ino`:

```cpp
uint8_t MIC_KEYS[2] = { KEY_SCROLL_LOCK, KEY_PAUSE };
```

Change to any key from Arduino's `Keyboard.h` (e.g. `KEY_F13`, `KEY_F14`), then recompile and upload.

## Web configuration UI

A web configuration page exists in `web/` (jQuery + WebUSB) but **WebUSB is not yet implemented in the firmware**. Keys can only be changed by editing the source and re-flashing.

## Architecture

All firmware logic is in a single sketch file: `MicrophoneController.ino`.

### Key constants

| Constant | Default | Purpose |
|----------|---------|---------|
| `HOLD_MS` | `50` | Milliseconds held before press is treated as PTT |
| `DEBOUNCE_MS` | `10` | Debounce window (ms) |
| `MIC_KEYS[2]` | `{ KEY_SCROLL_LOCK, KEY_PAUSE }` | Keyboard keys per button |
| `JOY_BTN[2]` | `{ 7, 8 }` | 0-indexed joystick button slots |

### Button state machine

| Field | Purpose |
|-------|---------|
| `pressed` | Current debounced press state |
| `hold` | True once hold threshold crossed |
| `latch` | Latched active state (toggled by tap, set/cleared by hold) |
| `pressStart` | `millis()` at press start |
| `lastDebounce` | `millis()` at last debounce check |

### Key functions

- `setSignal(idx, active)` — press/release key (keyboard) or set joystick bit (gamepad)
- `setMode()` — reads mode pin, switches mode if changed, releases all outputs
- `handleButton(idx)` — debounce, press/hold/release detection, latch management, LED update
- `gamepadSetButton(index, state)` — sets/clears single bit in 32-bit button state
- `gamepadSend()` — sends current button state as raw HID report (report ID 3)
- `initVariant()` — registers custom 32-button HID descriptor before USB enumeration

## Serial monitor

The device prints status messages at **9600 baud**:
- Mode changes: `Switched to mode: keyboard`

View with Arduino IDE Serial Monitor or `screen /dev/ttyACM0 9600`.

## Troubleshooting

### IntelliSense errors in VS Code / Arduino IDE

`#include` errors for `Keyboard.h` / `HID.h` are IntelliSense path issues only — code compiles correctly.

Fix: Command palette → **Arduino: Board Config** → select Leonardo → **Arduino: Rebuild IntelliSense Configuration**.

### Device not detected during upload

1. Verify a data cable (not power-only) is used
2. Check Tools → Board → Arduino Leonardo
3. If needed: disconnect, hold reset, reconnect, upload immediately

### Keys not responding

1. Check mode pin (D15) wiring
2. Test with a different application
3. Open Serial Monitor to verify mode detection
4. Check button wiring with a multimeter (should short to GND when pressed)
