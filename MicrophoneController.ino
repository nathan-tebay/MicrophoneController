// MicrophoneController
// Board: nullbits Bit-C PRO (ATmega32U4)
// Select "Arduino Leonardo" in the Arduino IDE board menu.
//
// Required libraries:
//   - Keyboard  (built-in, Arduino AVR core)
//   - HID       (built-in, Arduino AVR core) — no external joystick library needed
//   - EEPROM    (built-in, Arduino AVR core)
//   - WebUSB    (install via: arduino-cli lib install "WebUSB")
//
// Behavior (identical in both modes):
//   Tap  → latch toggle: signal activates and stays active; tap again to deactivate
//   Hold → PTT: signal active only while physically held; always OFF on release
//          (hold never changes the latch state)
//
// Config mode (WebUSB):
//   Press both buttons simultaneously 3× within 1 second to toggle config mode.
//   LEDs flash 3× on entry, 2× on exit. While active, the device accepts keycode
//   updates from the web config page at http://localhost:8080.
//   Simultaneous button presses do not toggle latches (reserved for this gesture).
//
// Wiring:
//   BTN1 → pin 2  (to GND, uses internal pull-up)
//   BTN2 → pin 3  (to GND, uses internal pull-up)
//   LED1 → pin 4  (through ~220Ω resistor to GND)
//   LED2 → pin 5  (through ~220Ω resistor to GND)
//   MODE → pin 15 (switch to GND = keyboard mode; open/HIGH = gamepad mode)

#include <Keyboard.h>
#include <HID.h>
#include <EEPROM.h>
#include <WebUSB.h>

// Config page served locally — run: python3 -m http.server 8080 (inside web/)
// https_only=0 allows http://localhost; change to 1 and update URL for HTTPS hosting.
WebUSB WebUSBSerial(0, "localhost:8080");

// --- Pins ---
const uint8_t BTN_PINS[2] = { 2, 3 };
const uint8_t LED_PINS[2] = { 4, 5 };
const uint8_t MODE_PIN    = 15;

// --- Gamepad HID (built-in AVR HID, no external library) ---
// 32-button gamepad, no axes. Active buttons are at 0-indexed positions 7 and 8,
// which the OS reports as buttons 8 and 9 — safely beyond all current
// standard and elite controllers (DualSense tops out at 14 HID buttons).
const uint8_t JOY_BTN[2] = { 7, 8 }; // 0-indexed; games see buttons 8 & 9
const uint8_t GAMEPAD_REPORT_ID = 3;  // 1=keyboard, 2=mouse already claimed

#ifndef KEY_SCROLL_LOCK
  #define KEY_SCROLL_LOCK 0xCF
#endif
#ifndef KEY_PAUSE
  #define KEY_PAUSE 0xE1
#endif

// --- EEPROM layout ---
// 0: magic (0xAB)  — confirms EEPROM written by this firmware
// 1: MIC_KEYS[0]   — button 0 keycode
// 2: MIC_KEYS[1]   — button 1 keycode
// 3: MIC_MODS[0]   — button 0 modifier mask (bit0=Ctrl, bit1=Shift, bit2=Alt)
// 4: MIC_MODS[1]   — button 1 modifier mask
#define EEPROM_MAGIC      0xAB
#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_KEY0  1
#define EEPROM_ADDR_KEY1  2
#define EEPROM_ADDR_MOD0  3
#define EEPROM_ADDR_MOD1  4

// Keycodes and modifier masks for each button in keyboard mode.
// Loaded from EEPROM at startup; compile-time values are the fallback defaults.
uint8_t MIC_KEYS[2] = { KEY_SCROLL_LOCK, KEY_PAUSE };
uint8_t MIC_MODS[2] = { 0, 0 }; // bit0=Ctrl, bit1=Shift, bit2=Alt

// --- Timing ---
const unsigned long HOLD_MS     = 50;  // ms to distinguish tap from hold
const unsigned long DEBOUNCE_MS = 10;

// --- Config mode combo ---
// Press both buttons simultaneously COMBO_REQUIRED times within COMBO_WINDOW_MS.
#define COMBO_REQUIRED  3
#define COMBO_WINDOW_MS 1000



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
// EEPROM helpers

void loadKeysFromEEPROM() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC) return; // use compile-time defaults
  MIC_KEYS[0] = EEPROM.read(EEPROM_ADDR_KEY0);
  MIC_KEYS[1] = EEPROM.read(EEPROM_ADDR_KEY1);
  MIC_MODS[0] = EEPROM.read(EEPROM_ADDR_MOD0);
  MIC_MODS[1] = EEPROM.read(EEPROM_ADDR_MOD1);
}

void saveButtonToEEPROM(uint8_t idx, uint8_t key, uint8_t mod) {
  EEPROM.update(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.update(idx == 0 ? EEPROM_ADDR_KEY0 : EEPROM_ADDR_KEY1, key);
  EEPROM.update(idx == 0 ? EEPROM_ADDR_MOD0 : EEPROM_ADDR_MOD1, mod);
}

// ---------------------------------------------------------------------------
// WebUSB config protocol
//
// Config mode toggled by pressing both buttons 3× within 1 s.
//
// Device → host on connect:  'C' <key0> <mod0> <key1> <mod1>   (5 bytes)
// Host → device:             'K' <idx> <keycode> <modmask>      (4 bytes)
// Host → device:             'R'                                 (1 byte, re-send config)
// Device → host (ack):       'A'                                 (1 byte)
//
// modmask bits: bit0=Ctrl, bit1=Shift, bit2=Alt

bool configMode = false;

void handleWebUSB() {
  static bool wasConnected   = false;
  static bool prevConfigMode = false;

  if (!configMode) {
    prevConfigMode = false;
    return;
  }

  // When config mode is freshly enabled, reset connection tracking so the device
  // re-announces its config to any already-connected host.
  if (!prevConfigMode) {
    wasConnected   = false;
    prevConfigMode = true;
  }

  bool connected = (bool)WebUSBSerial;

  if (connected && !wasConnected) {
    WebUSBSerial.write('C');
    WebUSBSerial.write(MIC_KEYS[0]);
    WebUSBSerial.write(MIC_MODS[0]);
    WebUSBSerial.write(MIC_KEYS[1]);
    WebUSBSerial.write(MIC_MODS[1]);
    WebUSBSerial.flush();
  }
  wasConnected = connected;

  if (!connected) return;

  while (WebUSBSerial.available() >= 1) {
    uint8_t cmd = WebUSBSerial.read();
    if (cmd == 'K' && WebUSBSerial.available() >= 3) {
      uint8_t idx = WebUSBSerial.read();
      uint8_t key = WebUSBSerial.read();
      uint8_t mod = WebUSBSerial.read();
      if (idx < 2) {
        MIC_KEYS[idx] = key;
        MIC_MODS[idx] = mod;
        saveButtonToEEPROM(idx, key, mod);
        WebUSBSerial.write('A');
        WebUSBSerial.flush();
      }
    } else if (cmd == 'R') {
      WebUSBSerial.write('C');
      WebUSBSerial.write(MIC_KEYS[0]);
      WebUSBSerial.write(MIC_MODS[0]);
      WebUSBSerial.write(MIC_KEYS[1]);
      WebUSBSerial.write(MIC_MODS[1]);
      WebUSBSerial.flush();
    }
  }
}

// ---------------------------------------------------------------------------
// Combo detection — both buttons pressed simultaneously COMBO_REQUIRED times
// within COMBO_WINDOW_MS ms toggles configMode.
//
// bothPressedActive is set for the duration of any simultaneous press so
// handleButton() can skip the latch-toggle (this gesture is reserved).

bool bothPressedActive = false;

void checkCombo() {
  static bool          prevBothPressed  = false;
  static uint8_t       comboCount       = 0;
  static unsigned long comboWindowStart = 0;

  bool bothPressed = buttons[0].pressed && buttons[1].pressed;

  if (bothPressed && !prevBothPressed) {
    unsigned long now = millis();
    if (comboCount == 0 || (now - comboWindowStart) > COMBO_WINDOW_MS) {
      comboCount       = 1;
      comboWindowStart = now;
    } else {
      comboCount++;
      if (comboCount >= COMBO_REQUIRED) {
        comboCount = 0;
        configMode = !configMode;
        Serial.print("Config mode: "); Serial.println(configMode ? "ON" : "OFF");

        // Flash both LEDs: 3× = enter config, 2× = exit
        uint8_t flashes = configMode ? 3 : 2;
        for (uint8_t f = 0; f < flashes; f++) {
          digitalWrite(LED_PINS[0], HIGH);
          digitalWrite(LED_PINS[1], HIGH);
          delay(80);
          digitalWrite(LED_PINS[0], LOW);
          digitalWrite(LED_PINS[1], LOW);
          delay(80);
        }
        // Restore LEDs to reflect current latch state
        for (int i = 0; i < 2; i++) {
          digitalWrite(LED_PINS[i], buttons[i].latch ? HIGH : LOW);
        }
      }
    }
    bothPressedActive = true;
  }

  if (!bothPressed && prevBothPressed) {
    bothPressedActive = false;
  }

  prevBothPressed = bothPressed;
}

// ---------------------------------------------------------------------------

String readMode(); // forward declaration (Arduino IDE won't auto-prototype String-returning functions)

String currentMode = "default";

struct Button {
  bool pressed          = false;
  bool hold             = false; // true once hold threshold crossed this press
  bool latch            = false; // latched active state (tap-toggle)
  unsigned long pressStart   = 0;
  unsigned long lastDebounce = 0;
};

Button buttons[2];

// --- Signal helper ---
// Activates or deactivates HID output for one button.
// Keyboard mode: presses/releases modifier keys then the main key.
// Gamepad mode:  sets or clears the joystick button bit.
void setSignal(int idx, bool active) {
  if (currentMode == "keyboard") {
    if (active) {
      if (MIC_MODS[idx] & 0x01) Keyboard.press(KEY_LEFT_CTRL);
      if (MIC_MODS[idx] & 0x02) Keyboard.press(KEY_LEFT_SHIFT);
      if (MIC_MODS[idx] & 0x04) Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(MIC_KEYS[idx]);
    } else {
      Keyboard.release(MIC_KEYS[idx]);
      if (MIC_MODS[idx] & 0x04) Keyboard.release(KEY_LEFT_ALT);
      if (MIC_MODS[idx] & 0x02) Keyboard.release(KEY_LEFT_SHIFT);
      if (MIC_MODS[idx] & 0x01) Keyboard.release(KEY_LEFT_CTRL);
    }
  } else {
    gamepadSetButton(JOY_BTN[idx], active);
  }
}

void setMode() {
  String newMode = readMode();
  if (newMode == currentMode) return;

  // Release all outputs for the outgoing mode before switching
  for (int i = 0; i < 2; i++) {
    buttons[i].pressed = false;
    buttons[i].hold    = false;
    buttons[i].latch   = false;
    digitalWrite(LED_PINS[i], LOW);
  }
  if (currentMode == "keyboard") {
    Keyboard.releaseAll(); // clears any held modifiers as well as the main keys
  } else {
    for (int i = 0; i < 2; i++) gamepadSetButton(JOY_BTN[i], false);
    gamepadSend();
  }

  currentMode = newMode;
  Serial.print("Switched to mode: "); Serial.println(currentMode);
}

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  delay(100); // Allow time for serial monitor to connect before printing mode

  loadKeysFromEEPROM();

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
  checkCombo();
  handleWebUSB();
  if (currentMode == "gamepad") {
    static uint32_t prevButtons = 0xFFFFFFFF; // force send on first loop
    if (_gamepadReport.buttons != prevButtons) {
      gamepadSend();
      prevButtons = _gamepadReport.buttons;
    }
  }
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

  // ── Hold threshold reached → PTT start ────────────────────────────────────
  if (btn.pressed && nowPressed && !btn.hold && (now - btn.pressStart >= HOLD_MS)) {
    btn.hold  = true;
    btn.latch = true;
  }

  if (btn.hold) {
    if (btn.pressed && !nowPressed) {
      // End hold/PTT on release
      btn.hold    = false;
      btn.pressed = false;
      btn.latch   = false;
    }
  } else {
    if (btn.pressed != nowPressed) {
      if (nowPressed) {
        btn.pressStart = now;
      }
      if (btn.pressed && !nowPressed) {
        // Button released — toggle latch on tap, unless this press was part of
        // a simultaneous both-button gesture (reserved for config mode combo).
        if (!bothPressedActive) {
          btn.latch = !btn.latch;
        }
      }
      btn.pressed = nowPressed;
    }
  }

  // Ensure signal tracks latch state
  setSignal(idx, btn.latch);
  digitalWrite(LED_PINS[idx], btn.latch ? HIGH : LOW);
}
