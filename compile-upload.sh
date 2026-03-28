#!/usr/bin/env bash
set -e

SKETCH=/mnt/LargeNVMe/Projects/GitHub/MicrophoneController
BUILD=$SKETCH/.build
HEX=$BUILD/MicrophoneController.ino.hex

check_deps() {
    if ! command -v arduino-cli &>/dev/null; then
        echo "arduino-cli not found. Install from https://arduino.github.io/arduino-cli/" >&2
        exit 1
    fi
    arduino-cli lib install "WebUSB" 2>/dev/null || true
    if ! command -v dfu-programmer &>/dev/null; then
        echo "dfu-programmer not found. Installing..."
        if command -v apt &>/dev/null; then
            sudo apt install -y dfu-programmer
        elif command -v dnf &>/dev/null; then
            sudo dnf install -y dfu-programmer
        else
            echo "No supported package manager found (apt, dnf). Install dfu-programmer manually." >&2
            exit 1
        fi
    fi
}

compile() {
    arduino-cli compile --fqbn arduino:avr:leonardo \
        --build-property 'build.usb_product="Microphone Controller"' \
        --build-property 'build.usb_manufacturer="Tebay.dev"' \
        --build-property 'build.vid=0x1209' \
        --build-property 'build.pid=0x0001' \
        --build-path "$BUILD" "$SKETCH"
}

upload() {
    dfu-programmer atmega32u4 erase
    dfu-programmer atmega32u4 flash "$HEX"
    dfu-programmer atmega32u4 reset
}

check_deps

case "${1:-all}" in
    compile) compile ;;
    upload)  upload ;;
    all)     compile && upload ;;
    *) echo "Usage: $0 [compile|upload|all]" >&2; exit 1 ;;
esac
