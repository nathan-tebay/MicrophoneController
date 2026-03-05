# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

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

No external libraries required. Gamepad HID output is implemented directly via the AVR core's `HID.h` (`HIDSubDescriptor` + `HID().SendReport()`), so no third-party joystick library is needed.

## Commands

| Task | How |
|------|-----|
| Compile | Arduino IDE → Sketch → Verify/Compile (`Ctrl+R`) |
| Upload | Arduino IDE → Sketch → Upload (`Ctrl+U`) |
| Serial monitor | Tools → Serial Monitor — 9600 baud (prints mode changes; unused at runtime otherwise) |
| `arduino-cli` compile | `arduino-cli compile --fqbn arduino:avr:leonardo MicrophoneController` |
| `arduino-cli` upload | `arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:leonardo MicrophoneController` |

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
| `MIC_KEYS[2]` | `{ KEY_SCROLL_LOCK, KEY_PAUSE }` | Keyboard keys for each button |
| `JOY_BTN[2]` | `{ 7, 8 }` | 0-indexed joystick button slots (OS reports as buttons 8 and 9) |

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
- `setSignal(idx, active)` — activates or deactivates HID output for one button (keyboard key or gamepad button depending on current mode)
- `setMode()` — reads mode pin, switches mode if changed, releases all outputs and resets all button state on transition
- `readMode()` — reads `MODE_PIN` and returns `"keyboard"` or `"gamepad"`
- `handleButton(idx)` — debounce, press/hold/release detection, latch management, HID output, LED update for one button

## VS Code / IntelliSense

The `#include` errors for `Keyboard.h` and `Joystick.h` are IntelliSense path issues only — the code compiles correctly from the Arduino IDE.
To resolve squiggles: open the command palette and run **Arduino: Board Config**, select **Arduino Leonardo**, then run **Arduino: Rebuild IntelliSense Configuration**.
