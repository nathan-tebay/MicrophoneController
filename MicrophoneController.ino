// MicrophoneController
// Board: nullbits Bit-C PRO (ATmega32U4)
// Select "Arduino Leonardo" in the Arduino IDE board menu.
//
// Required libraries:
//   - Keyboard  (built-in, Arduino AVR core)
//   - HID       (built-in, Arduino AVR core) — no external joystick library needed
//
// Behavior (identical in both modes):
//   Tap  → latch toggle: signal activates and stays active; tap again to deactivate
//   Hold → PTT: signal active only while physically held; always OFF on release
//          (hold never changes the latch state)
//
// Wiring:
//   BTN1 → pin 2  (to GND, uses internal pull-up)
//   BTN2 → pin 3  (to GND, uses internal pull-up)
//   LED1 → pin 4  (through ~220Ω resistor to GND)
//   LED2 → pin 5  (through ~220Ω resistor to GND)
//   MODE → pin 15 (switch to GND = keyboard mode; open/HIGH = gamepad mode)

#include <Keyboard.h>
#include <HID.h>

// --- Pins ---
const uint8_t BTN_PINS[2] = { 2, 3 };
const uint8_t LED_PINS[2] = { 4, 5 };
const uint8_t MODE_PIN    = 15;
// --- Gamepad HID (built-in AVR HID, no external library) ---
// 32-button gamepad, no axes. Active buttons are at 0-indexed positions 19 and
// 20, which games report as buttons 20 and 21 — safely beyond all current
// standard and elite controllers (DualSense tops out at 14 HID buttons).
const uint8_t JOY_BTN[2] = { 7, 8 }; // 0-indexed; games see buttons 8 & 9
const uint8_t GAMEPAD_REPORT_ID = 3;    // 1=keyboard, 2=mouse already claimed

#ifndef KEY_SCROLL_LOCK
  #define KEY_SCROLL_LOCK 0xCF
#endif
#ifndef KEY_PAUSE
  #define KEY_PAUSE 0xE1
#endif

const uint8_t MIC_KEYS[2] = { KEY_SCROLL_LOCK, KEY_PAUSE };

// --- Timing ---
const unsigned long HOLD_MS     = 50;  // ms to distinguish tap from hold
const unsigned long DEBOUNCE_MS = 10;



static const uint8_t _gamepadDescriptor[] PROGMEM = {
  0x05, 0x01,              // Usage Page (Generic Desktop)
  0x09, 0x04,              // Usage (Joystick) — 0x04=Joystick; more widely recognized
                           //   than 0x05=Game Pad by joydev and controller software
  0xA1, 0x01,              // Collection (Application)
  0x85, GAMEPAD_REPORT_ID, // Report ID
  // X and Y axes — joydev requires at least one ABS axis to expose /dev/input/jsX.
  // Unsigned 0–255, always sent as 128 (center). Avoids signed-byte encoding
  // ambiguity (0x15,0x81 can be misread as unsigned 129 > max, failing validation).
  0x09, 0x30,              //   Usage (X)
  0x09, 0x31,              //   Usage (Y)
  0x15, 0x00,              //   Logical Minimum (0)
  0x26, 0xFF, 0x00,        //   Logical Maximum (255) — 2-byte encoding
  0x75, 0x08,              //   Report Size (8 bits)
  0x95, 0x02,              //   Report Count (2)
  0x81, 0x02,              //   Input (Data, Variable, Absolute)
  // 32 buttons, 1 bit each = 4 bytes
  0x05, 0x09,              //   Usage Page (Buttons)
  0x19, 0x01,              //   Usage Minimum (1)
  0x29, 0x20,              //   Usage Maximum (32)
  0x15, 0x00,              //   Logical Minimum (0)
  0x25, 0x01,              //   Logical Maximum (1)
  0x75, 0x01,              //   Report Size (1 bit)
  0x95, 0x20,              //   Report Count (32)
  0x81, 0x02,              //   Input (Data, Variable, Absolute)
  0xC0                     // End Collection
};

// Report layout: [x: uint8_t][y: uint8_t][buttons: uint32_t] = 6 bytes
struct __attribute__((packed)) GamepadReport {
  uint8_t  x;       // always 128 (center of 0–255 range)
  uint8_t  y;       // always 128 (center of 0–255 range)
  uint32_t buttons;
};
static GamepadReport _gamepadReport = { 128, 128, 0 };
// initVariant() is called by Arduino's main() after init() but before
// USBDevice.attach(). Overriding it here is the only reliable way to register
// a custom HID descriptor before USB enumeration occurs — mirroring exactly
// what the Keyboard and Mouse libraries do in their global constructors.
void initVariant() {
  static HIDSubDescriptor node(_gamepadDescriptor, sizeof(_gamepadDescriptor));
  HID().AppendDescriptor(&node);
}

void gamepadSetButton(uint8_t index, bool state) {
  if (index > 31) return;
  if (state) _gamepadReport.buttons |=  (1UL << index);
  else       _gamepadReport.buttons &= ~(1UL << index);
}

void gamepadSend() {
  _gamepadReport.x = 128; // keep axes centered
  _gamepadReport.y = 128;
  HID().SendReport(GAMEPAD_REPORT_ID, &_gamepadReport, sizeof(_gamepadReport));
}

// ---------------------------------------------------------------------------

String readMode(); // forward declaration (Arduino IDE won't auto-prototype String-returning functions)

String currentMode = "default";

struct Button {
  bool pressed          = false;
  bool hold        = false; // true once hold threshold crossed this press
  bool latch            = false; // latched active state (tap-toggle)
  unsigned long pressStart   = 0;
  unsigned long lastDebounce = 0;
};

Button buttons[2];

// --- Signal helper ---
// Activates or deactivates the HID output for one button.
// Keyboard mode: holds or releases the key.
// Gamepad mode:  sets or clears the joystick button bit.
void setSignal(int idx, bool active) {
  if (currentMode == "keyboard") {
    if (active) Keyboard.press(MIC_KEYS[idx]);
    else Keyboard.release(MIC_KEYS[idx]);
  } else {
    gamepadSetButton(JOY_BTN[idx], active);
  }
}

void setMode() {
  String newMode = readMode();
  if (newMode == currentMode) return;

  // Release all outputs for the outgoing mode before switching
  for (int i = 0; i < 2; i++) {
    if (currentMode == "keyboard") Keyboard.release(MIC_KEYS[i]);
    else gamepadSetButton(JOY_BTN[i], false);
    buttons[i].pressed = false;
    buttons[i].hold    = false;
    buttons[i].latch   = false;
    digitalWrite(LED_PINS[i], LOW);
  }
  if (currentMode == "gamepad") gamepadSend();

  currentMode = newMode;
  Serial.print("Switched to mode: "); Serial.println(currentMode);
}

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  delay(100); // Allow time for serial monitor to connect before printing mode
    
  for (int i = 0; i < 2; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  pinMode(MODE_PIN, INPUT_PULLUP);
  setMode(); // initializes currentMode and HID state
  // Push an initial gamepad report so the kernel registers the joystick
  // device and its initial state before the first loop iteration.
  gamepadSend();
}

void loop() {
  setMode(); // also checks for mode changes each loop
  for (int i = 0; i < 2; i++) {
    handleButton(i);
  }
  if (currentMode == "gamepad") gamepadSend();
}

// ---------------------------------------------------------------------------

String readMode() {
  return (digitalRead(MODE_PIN) == LOW) ? "keyboard" : "gamepad";
}

void handleButton(int idx) {
  Button& btn       = buttons[idx];
  unsigned long now = millis();

  if ((now - btn.lastDebounce) < DEBOUNCE_MS) return;
  btn.lastDebounce = now;

  bool nowPressed = (digitalRead(BTN_PINS[idx]) == LOW);
  // ── Hold threshold reached → PTT start ───────────────────────────────────
  if (btn.pressed && nowPressed && !btn.hold && (now - btn.pressStart >= HOLD_MS)) {
    btn.hold = true;
    btn.pressed = true; 
    btn.latch = true;
  }

  if (btn.hold) {
    if(btn.pressed && !nowPressed) {
      // End hold/PTT on release
      btn.hold = false;
      btn.pressed = false; 
      btn.latch = false; // Clear latch on release to ensure tap state is off
    }
  } else {
    if (btn.pressed != nowPressed) {
      if(nowPressed) {
        btn.pressStart = now; // Start timing for hold detection
      } 
      if(btn.pressed && !nowPressed) {
        // Button released → toggle latch on tap
        btn.latch = !btn.latch;
      }
      btn.pressed = nowPressed; 
    }
  }

  // Ensure signal is active if latch is on, even if hold state changes
  setSignal(idx, btn.latch);
  digitalWrite(LED_PINS[idx], btn.latch ? HIGH : LOW);
}

