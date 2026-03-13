# MicrophoneController

A 2-button USB HID microphone controller built on the **nullbits Bit-C PRO (ATmega32U4)** microcontroller. Each button supports tap-to-toggle and hold-for-push-to-talk (PTT). A mode switch selects between keyboard and gamepad HID output modes. Dual LEDs reflect the active state of each microphone button.

## Features

- **Dual-mode HID output**: Switch between keyboard (Scroll Lock / Pause keys) and gamepad (buttons 8–9) modes
- **Tap-to-toggle**: Press once to activate a microphone latch that persists until toggled off
- **Push-to-talk (PTT)**: Hold a button to send a signal only while physically held; releases on button release
- **Per-button configuration**: Remap keys and add modifiers (Ctrl, Shift, Alt) via WebUSB
- **Non-volatile storage**: Configuration persists across power cycles (stored in EEPROM)
- **LED indicators**: Each button has an LED that reflects its active state
- **Web configuration UI**: Chrome-based configuration page with real-time keycode selection and preview

## Hardware Requirements

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

## Software Requirements

### Toolchain
- **Arduino IDE**: Latest version (or `arduino-cli`)
- **Board package**: Arduino AVR (built-in)
- **Upload protocol**: USB (native ATmega32U4 USB bootloader)

### Libraries
| Library | Source | Installation |
|---------|--------|--------------|
| `Keyboard` | Built-in (Arduino AVR core) | — |
| `HID` | Built-in (Arduino AVR core) | — |
| `EEPROM` | Built-in (Arduino AVR core) | — |
| `WebUSB` | Arduino library manager | `arduino-cli lib install "WebUSB"` |

**Note**: Gamepad HID output is implemented directly via the AVR core's `HID.h` (`HIDSubDescriptor` + `HID().SendReport()`), so no third-party joystick library is needed.

## Build & Upload

### Using the Build Script (Recommended)

The `compile-upload.sh` script automates the entire process:

```bash
# Compile and upload (full cycle)
./compile-upload.sh

# Compile only
./compile-upload.sh compile

# Upload only
./compile-upload.sh upload
```

The script:
- Ensures `dfu-programmer` is installed (auto-installs via `apt` or `dnf` if missing)
- Ensures the `WebUSB` library is available
- Compiles with custom USB identifiers (VID `0x1209`, PID `0x0001`)
- Uses `dfu-programmer` for flashing: erase → flash → reset
- Stores build artifacts in `.build/`

### Using Arduino IDE

1. Open **MicrophoneController.ino** in the Arduino IDE
2. Select **Tools → Board → Arduino AVR Boards → Arduino Leonardo**
3. **Compile**: Sketch → Verify/Compile (`Ctrl+R`)
4. **Upload**: Sketch → Upload (`Ctrl+U`)

### Manual CLI Compilation

```bash
arduino-cli compile --fqbn arduino:avr:leonardo --build-path .build MicrophoneController
```

## Button Behavior

### In Keyboard Mode (Mode pin LOW)

| Action | Behavior |
|--------|----------|
| **Tap** | Toggle microphone latch state; LED tracks latch |
| **Hold** (≥50 ms) | Push-to-talk: signal active only while held; signal and latch clear on release |
| **Mode switch** | All keys released; latch states reset; mode indicator printed to serial |

**Default key assignments**:
- Button 1: `Scroll Lock` (0xCF)
- Button 2: `Pause / Break` (0xE1)

These keys are chosen because they are vestigial in modern software and fully supported for PTT in Discord, TeamSpeak, and Mumble.

### In Gamepad Mode (Mode pin HIGH)

| Action | Behavior |
|--------|----------|
| **Tap** | Toggle joystick button latch state; LED tracks latch |
| **Hold** (≥50 ms) | Push-to-talk: joystick button active only while held; clears on release |
| **Mode switch** | All joystick buttons released; latch states reset; mode indicator printed to serial |

**Button assignments**:
- Button 1: Joystick button 8 (OS reports as button 8, 0-indexed slot 7)
- Button 2: Joystick button 9 (OS reports as button 9, 0-indexed slot 8)

These button slots are safely beyond the standard controller layout (4–6 face buttons on most controllers, max 14 on PS5 DualSense) and avoid shadowing gameplay bindings.

## Configuration & WebUSB

### Entering Configuration Mode

Press **both buttons simultaneously 3 times within 1 second**. The LEDs will flash 3 times to confirm entry. While in config mode, the WebUSB interface becomes available.

**Exiting configuration mode**: Press both buttons simultaneously 3 times within 1 second again. The LEDs will flash 2 times to confirm exit.

### Web Configuration Page

#### Setup

1. **Ensure the device is in config mode** (LEDs flash 3×)
2. **Start a local web server** from the `web/` directory:
   ```bash
   cd web && python3 -m http.server 8080
   ```
3. **Open in Chrome** (Firefox and Safari do not support WebUSB):
   ```
   http://localhost:8080
   ```
4. **Click "Connect Device"** and select your controller from the system dialog

#### Features

- **Key selection**: Dropdown menus organized by category (Special, Function, Navigation, Letters, Numbers)
- **Modifier selection**: Checkboxes for Ctrl, Shift, Alt
- **Live preview**: See the combined keycode (e.g., "Ctrl + Shift + F1") before saving
- **Real-time sync**: Changes saved to the device are automatically written to EEPROM

#### WebUSB Protocol

Communication uses raw bytes over the WebUSB bulk interface:

| Direction | Bytes | Meaning |
|-----------|-------|---------|
| Device → host (on connect or `R` command) | `'C' <key0> <mod0> <key1> <mod1>` | Current config (5 bytes) |
| Host → device | `'K' <idx> <keycode> <modmask>` | Set button `idx` (0 or 1); saves to EEPROM; device replies `'A'` |
| Host → device | `'R'` | Request config re-send (1 byte) |

**Modifier mask bits**:
- bit0 (0x01) = Ctrl
- bit1 (0x02) = Shift
- bit2 (0x04) = Alt

#### HTTPS Hosting

To host the configuration page over HTTPS:
1. Change `https_only` from `0` to `1` in [MicrophoneController.ino](MicrophoneController.ino) line 36
2. Update the WebUSB landing page URL to your HTTPS domain
3. Recompile and upload

## Architecture

All firmware logic resides in a single sketch file: [MicrophoneController.ino](MicrophoneController.ino).

### Key Constants

| Constant | Default | Purpose |
|----------|---------|---------|
| `HOLD_MS` | `50` | Milliseconds held before a press is treated as a hold/PTT |
| `DEBOUNCE_MS` | `10` | Debounce window in milliseconds |
| `MIC_KEYS[2]` | `{ KEY_SCROLL_LOCK, KEY_PAUSE }` | Keyboard keys per button; loaded from EEPROM at startup |
| `MIC_MODS[2]` | `{ 0, 0 }` | Modifier bitmask per button (bit0=Ctrl, bit1=Shift, bit2=Alt); loaded from EEPROM |
| `JOY_BTN[2]` | `{ 7, 8 }` | 0-indexed joystick button slots (OS reports as buttons 8 and 9) |
| `COMBO_REQUIRED` | `3` | Number of simultaneous both-button presses to toggle config mode |
| `COMBO_WINDOW_MS` | `1000` | Time window (ms) for COMBO_REQUIRED presses to be recognized |

### EEPROM Layout

| Address | Content | Notes |
|---------|---------|-------|
| `0` | Magic byte `0xAB` | Absent = EEPROM uninitialized; compile-time defaults are used |
| `1` | `MIC_KEYS[0]` keycode | Button 1 keycode |
| `2` | `MIC_KEYS[1]` keycode | Button 2 keycode |
| `3` | `MIC_MODS[0]` modifier mask | Button 1 modifiers |
| `4` | `MIC_MODS[1]` modifier mask | Button 2 modifiers |

**Note**: `EEPROM.update()` is used (skip-if-same) to minimize write cycles.

### Button State Machine

The `Button` struct tracks per-button state:

| Field | Type | Purpose |
|-------|------|---------|
| `pressed` | `bool` | Current debounced press state |
| `hold` | `bool` | True once hold threshold (≥50 ms) is crossed this press |
| `latch` | `bool` | Latched active state; toggled by tap, set/cleared by hold |
| `pressStart` | `ulong` | `millis()` at press start; used for hold detection |
| `lastDebounce` | `ulong` | `millis()` at last debounce check |

### Key Functions

- `initVariant()` — Registers the custom 32-button HID descriptor before USB enumeration (called by Arduino's `main()` after `init()`, before `USBDevice.attach()`)
- `gamepadSetButton(index, state)` — Sets or clears a single bit in the 32-bit button state word
- `gamepadSend()` — Sends the current button state as a raw HID report (report ID 3)
- `loadKeysFromEEPROM()` — Reads keycodes and modifier masks from EEPROM; no-op if magic byte absent
- `saveButtonToEEPROM(idx, key, mod)` — Writes magic byte, keycode, and modifier mask using `update()` (skip-if-same)
- `handleWebUSB()` — Gated on `configMode`; tracks connection state, announces config on connect, dispatches `K`/`R` commands
- `checkCombo()` — Detects simultaneous both-button presses; sets `bothPressedActive`; toggles `configMode` + flashes LEDs on 3rd press within window
- `setSignal(idx, active)` — Presses/releases modifier keys then main key (keyboard mode) or sets joystick bit (gamepad mode)
- `setMode()` — Reads mode pin, switches mode if changed, releases all outputs, and resets all button state on transition
- `readMode()` — Reads `MODE_PIN` and returns `"keyboard"` (LOW) or `"gamepad"` (HIGH)
- `handleButton(idx)` — Debounce, press/hold/release detection, latch management, HID output, and LED update for one button

## Serial Monitor

The device prints status messages to the serial port at **9600 baud**:

- Mode changes (e.g., "Switched to mode: keyboard")
- Config mode toggles (e.g., "Config mode: ON")

To view:
1. **Arduino IDE**: Tools → Serial Monitor
2. **CLI**: `screen /dev/ttyACM0 9600` or similar

## Remapping Keys

### Via WebUSB (Recommended)

1. Enter config mode (press both buttons 3× within 1 s; LEDs flash 3×)
2. Serve the config page: `cd web && python3 -m http.server 8080`
3. Open http://localhost:8080 in Chrome
4. Click "Connect Device"
5. Select your desired key and modifiers, then click "Save Button 1" or "Save Button 2"

### Via Code Modification

1. Edit [MicrophoneController.ino](MicrophoneController.ino), line 72:
   ```cpp
   uint8_t MIC_KEYS[2] = { KEY_SCROLL_LOCK, KEY_PAUSE };
   ```
2. Change to your desired keys (refer to Arduino's `Keyboard.h` for available constants)
3. Recompile and upload

**Available key constants** (Arduino Keyboard library):
- Special: `KEY_SCROLL_LOCK`, `KEY_PAUSE`, `KEY_CAPS_LOCK`, `KEY_PRINT_SCREEN`
- Function: `KEY_F1` through `KEY_F12`
- Navigation: `KEY_INSERT`, `KEY_DELETE`, `KEY_HOME`, `KEY_END`, `KEY_PAGE_UP`, `KEY_PAGE_DOWN`, `KEY_UP_ARROW`, `KEY_DOWN_ARROW`, `KEY_LEFT_ARROW`, `KEY_RIGHT_ARROW`
- Letters: Any letter `a`–`z` (keyboard library uses ASCII codes)
- Numbers: Any digit `0`–`9`
- Modifiers (usable in `MIC_MODS[2]`): `KEY_LEFT_CTRL`, `KEY_LEFT_SHIFT`, `KEY_LEFT_ALT`

## Troubleshooting

### IntelliSense Errors in VS Code / Arduino IDE

The `#include` errors for `Keyboard.h` and `HID.h` are IntelliSense path issues only — the code compiles correctly.

**To resolve**:
1. Open the command palette
2. Run **Arduino: Board Config**
3. Select **Arduino Leonardo**
4. Run **Arduino: Rebuild IntelliSense Configuration**

### Device Not Detected During Upload

1. **Check USB connection**: Ensure a data cable (not power-only) is used
2. **Verify board selection**: Arduino IDE → Tools → Board → Arduino Leonardo
3. **Check bootloader**: If the device won't enter bootloader, try:
   - Disconnect the device
   - Reconnect while pressing the reset button on the board
4. **Manual bootloader entry** (if needed):
   - Reset the device by grounding the reset pin briefly
   - Upload should begin immediately after

### WebUSB Connection Issues

- **Chrome required**: Firefox and Safari do not support WebUSB; use Chrome
- **Config mode not active**: Ensure LEDs flash 3× when you press both buttons 3 times within 1 second
- **Permissions denied**: On Linux, you may need to add a udev rule for the device (VID `0x1209`, PID `0x0001`)

### Keys Not Responding

1. **Check the mode pin (D15)**: Verify it's wired correctly
   - LOW (to GND) = keyboard mode
   - HIGH (floating) = gamepad mode
2. **Test with a different application**: Some apps don't respond to certain keys
3. **Check serial output**: Open the Serial Monitor to see if mode changes are being detected
4. **Verify button wiring**: Use a multimeter to check that buttons properly short to GND when pressed

## License

See LICENSE file for details.

## Contributing

For development guidance, refer to [CLAUDE.md](CLAUDE.md).
