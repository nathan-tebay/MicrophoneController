# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A 2-button USB HID microphone controller built on the **nullbits Bit-C PRO (ATmega32U4)**.
Each button supports tap-to-toggle and hold-for-push-to-talk. A mode switch selects between keyboard and gamepad HID output. LEDs reflect each mic's state.

## Board & Toolchain

| Item | Value |
|------|-------|
| MCU | ATmega32U4 |
| Arduino IDE board selection | **Arduino Leonardo** |
| Board package | Arduino AVR (built-in) |
| Upload protocol | USB (native ATmega32U4 USB bootloader) |

## Required Libraries

| Library | Source |
|---------|--------|
| `Keyboard` | Built-in (Arduino AVR core) |
| `HID` | Built-in (Arduino AVR core) |
| `EEPROM` | Built-in (Arduino AVR core) |
| `WebUSB` | Arduino library manager — `arduino-cli lib install "WebUSB"` (auto-installed by `compile-upload.sh`) |

Gamepad HID output is implemented directly via the AVR core's `HID.h` (`HIDSubDescriptor` + `HID().SendReport()`), so no third-party joystick library is needed.

## Commands

| Task | How |
|------|-----|
| Compile | Arduino IDE → Sketch → Verify/Compile (`Ctrl+R`) |
| Upload | Arduino IDE → Sketch → Upload (`Ctrl+U`) |
| Serial monitor | Tools → Serial Monitor — 9600 baud (prints mode changes; unused at runtime otherwise) |
| Compile + upload (script) | `./compile-upload.sh` (compile only: `./compile-upload.sh compile`, upload only: `./compile-upload.sh upload`) |
| `arduino-cli` compile only | `arduino-cli compile --fqbn arduino:avr:leonardo --build-path .build MicrophoneController` |

The `compile-upload.sh` script is the preferred CLI workflow. It uses `arduino-cli` for compilation (with custom USB VID `0x1209`, PID `0x0001`, product name `"Microphone Controller"`, manufacturer `"Tebay.dev"`) and `dfu-programmer` for flashing (erase → flash → reset). The script auto-installs `dfu-programmer` via `apt` or `dnf` if missing. Build artifacts go to `.build/`.

## Hardware

### Wiring

| Signal | Pin | Notes |
|--------|-----|-------|
| Button 1 | D2 | To GND; internal pull-up enabled |
| Button 2 | D3 | To GND; internal pull-up enabled |
| LED 1 | D4 | ~220 Ω resistor to GND |
| LED 2 | D5 | ~220 Ω resistor to GND |
| Mode switch | D15 | To GND = keyboard mode; floating/HIGH = gamepad mode |

### Button behavior

| Action | Keyboard mode | Gamepad mode |
|--------|---------------|--------------|
| Tap | Toggle mic latch (signal activates/deactivates; LED tracks latch state) | Toggle joystick button latch |
| Hold (≥ 50 ms) | PTT — sets latch active while held; clears latch and signal on release | Joystick button held; latch cleared on release |
| Mode switch change | All keys released; all latch states reset | All joystick buttons released; latch states reset |

## Key / Button Assignments

### Keyboard mode

| Button | Key | Rationale |
|--------|-----|-----------|
| 1 | `Scroll Lock` | Vestigial key, never bound by default in any major game engine or title; fully supported for PTT in Discord, TeamSpeak, Mumble |
| 2 | `Pause` | Vestigial key, almost never bound in games or standard software; fully supported by Discord, TeamSpeak, Mumble |

To remap, change `MIC_KEYS[2]` in [MicrophoneController.ino](MicrophoneController.ino).

### Gamepad mode

Joystick buttons **8 and 9** (1-indexed as reported to the OS/games).
The device declares 32 buttons in its HID descriptor; the two active buttons use 0-indexed slots 7 and 8.

**Why buttons 8–9:** Most standard controllers use 4–6 face buttons; the highest count on any shipping consumer controller is 14 (PS5 DualSense). Buttons 8–9 are safely beyond common default gameplay bindings and will not shadow standard controller layouts.

## Architecture

All logic lives in a single sketch file: [MicrophoneController.ino](MicrophoneController.ino).

### Key constants

| Constant | Default | Purpose |
|----------|---------|---------|
| `HOLD_MS` | `50` | Milliseconds held before a press is treated as a hold/PTT |
| `DEBOUNCE_MS` | `10` | Debounce window in milliseconds |
| `MIC_KEYS[2]` | `{ KEY_SCROLL_LOCK, KEY_PAUSE }` | Keyboard keys — loaded from EEPROM at startup; compile-time values are the fallback defaults |
| `MIC_MODS[2]` | `{ 0, 0 }` | Modifier bitmask per button (bit0=Ctrl, bit1=Shift, bit2=Alt) — loaded from EEPROM |
| `JOY_BTN[2]` | `{ 7, 8 }` | 0-indexed joystick button slots (OS reports as buttons 8 and 9) |
| `COMBO_REQUIRED` | `3` | Simultaneous both-button presses needed to toggle config mode |
| `COMBO_WINDOW_MS` | `1000` | Window in ms within which `COMBO_REQUIRED` presses must occur |

### EEPROM layout

| Address | Content |
|---------|---------|
| `0` | Magic byte `0xAB` — absent means EEPROM uninitialised; compile-time defaults are used |
| `1` | `MIC_KEYS[0]` keycode |
| `2` | `MIC_KEYS[1]` keycode |
| `3` | `MIC_MODS[0]` modifier mask |
| `4` | `MIC_MODS[1]` modifier mask |

`EEPROM.update()` is used (skip-if-same) to minimise write cycles.

### Config mode / WebUSB

Config mode is toggled by pressing **both buttons simultaneously 3× within 1 second**. LEDs flash 3× on entry, 2× on exit. Simultaneous button presses never toggle latches (the gesture is reserved).

The config page lives in `web/index.html`. Serve it locally:
```
cd web && python3 -m http.server 8080
# then open http://localhost:8080 in Chrome
```
The WebUSB landing page URL in the firmware constructor must match (currently `localhost:8080`). For HTTPS hosting change `https_only` from `0` to `1` and update the URL.

Chrome (not Firefox/Safari) is required for WebUSB. The page auto-discovers the vendor-specific USB interface (class `0xFF`) and its bulk endpoints — no hardcoded endpoint numbers.

**Protocol** — raw bytes over the WebUSB bulk interface:

| Direction | Bytes | Meaning |
|-----------|-------|---------|
| Device → host (on connect or `R`) | `'C' <key0> <mod0> <key1> <mod1>` | Current config (5 bytes) |
| Host → device | `'K' <idx> <keycode> <modmask>` | Set button `idx` (0 or 1); saves to EEPROM; device replies `'A'` |
| Host → device | `'R'` | Request config re-send (1 byte) |

### `Button` struct fields

| Field | Type | Purpose |
|-------|------|---------|
| `pressed` | `bool` | Current debounced press state |
| `hold` | `bool` | True once hold threshold was crossed this press |
| `latch` | `bool` | Latched active state; toggled by tap, set/cleared by hold |
| `pressStart` | `ulong` | `millis()` at press start, used for hold detection |
| `lastDebounce` | `ulong` | `millis()` at last debounce check |

### Key functions

- `initVariant()` — registers the custom 32-button HID descriptor before USB enumeration (called by Arduino's main() after init(), before USBDevice.attach())
- `gamepadSetButton(index, state)` — sets/clears a single bit in the 32-bit button state word
- `gamepadSend()` — sends the current button state as a raw HID report (report ID 3)
- `loadKeysFromEEPROM()` — reads keycodes and modifier masks from EEPROM; no-op if magic byte absent
- `saveButtonToEEPROM(idx, key, mod)` — writes magic, keycode, and modifier mask using `update()` (skip-if-same)
- `handleWebUSB()` — gated on `configMode`; tracks connection state, announces config on connect, dispatches `K`/`R` commands
- `checkCombo()` — detects simultaneous both-button presses; sets `bothPressedActive`; toggles `configMode` + flashes LEDs on 3rd press within window
- `setSignal(idx, active)` — presses/releases modifier keys then main key (keyboard mode) or sets joystick bit (gamepad mode)
- `setMode()` — reads mode pin, switches mode if changed, releases all outputs and resets all button state on transition
- `readMode()` — reads `MODE_PIN` and returns `"keyboard"` or `"gamepad"`
- `handleButton(idx)` — debounce, press/hold/release detection, latch management, HID output, LED update for one button

## VS Code / IntelliSense

The `#include` errors for `Keyboard.h` and `HID.h` are IntelliSense path issues only — the code compiles correctly from the Arduino IDE.
To resolve squiggles: open the command palette and run **Arduino: Board Config**, select **Arduino Leonardo**, then run **Arduino: Rebuild IntelliSense Configuration**.
