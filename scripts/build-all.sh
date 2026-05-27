#!/bin/bash

FQBN="arduino:avr:nano:cpu=atmega328"
PROJECT_DIR="$(dirname "$0")/../client"
CONTROLLER_DIR="$(dirname "$0")/../controller"
BIN_DIR="$(dirname "$0")/../bin"

mkdir -p "$BIN_DIR"

# Step 1: Build client variants with MODBUS_ADDRESS 1–10
for i in {1..10}; do
    echo "Building client for MODBUS_ADDRESS=$i..."

    TEMP_DIR=$(mktemp -d)
    cp -r "$PROJECT_DIR/"* "$TEMP_DIR/"

    # Rename .ino file to match sketch folder name
    ORIGINAL_INO=$(find "$TEMP_DIR" -maxdepth 1 -name "*.ino" | head -n 1)
    if [ -z "$ORIGINAL_INO" ]; then
        echo "Error: No .ino file found in client project!"
        rm -rf "$TEMP_DIR"
        exit 1
    fi

    TEMP_INO="$TEMP_DIR/$(basename "$TEMP_DIR").ino"
    mv "$ORIGINAL_INO" "$TEMP_INO"

    # Replace MODBUS_ADDRESS
    sed -i "s/#define MODBUS_ADDRESS .*/#define MODBUS_ADDRESS $i \/\/ Slave address/" "$TEMP_INO"

    # Compile
    arduino-cli compile --fqbn "$FQBN" -e "$TEMP_DIR"
    if [ $? -ne 0 ]; then
        echo "Compilation failed for MODBUS_ADDRESS=$i"
        rm -rf "$TEMP_DIR"
        continue
    fi

    HEX_FILE=$(find "$TEMP_DIR/build" -name "*.hex" | head -n 1)
    if [ -z "$HEX_FILE" ]; then
        echo "Error: No .hex file produced for MODBUS_ADDRESS=$i"
        rm -rf "$TEMP_DIR"
        continue
    fi

    OUTPUT_FILE="$BIN_DIR/claycast-client-$(printf "%02d" $i).hex"
    cp "$HEX_FILE" "$OUTPUT_FILE"
    echo "Saved build to $OUTPUT_FILE"

    rm -rf "$TEMP_DIR"
done

# Step 2: Build controller (no MODBUS_ADDRESS changes)
echo "Building controller..."

TEMP_DIR=$(mktemp -d)
cp -r "$CONTROLLER_DIR/"* "$TEMP_DIR/"

ORIGINAL_INO=$(find "$TEMP_DIR" -maxdepth 1 -name "*.ino" | head -n 1)
if [ -z "$ORIGINAL_INO" ]; then
    echo "Error: No .ino file found in controller project!"
    rm -rf "$TEMP_DIR"
    exit 1
fi

TEMP_INO="$TEMP_DIR/$(basename "$TEMP_DIR").ino"
mv "$ORIGINAL_INO" "$TEMP_INO"

arduino-cli compile --fqbn "$FQBN" -e "$TEMP_DIR"
if [ $? -ne 0 ]; then
    echo "Controller compilation failed"
    rm -rf "$TEMP_DIR"
    exit 1
fi

HEX_FILE=$(find "$TEMP_DIR/build" -name "*.hex" | head -n 1)
if [ -z "$HEX_FILE" ]; then
    echo "Error: No .hex file produced for controller"
    rm -rf "$TEMP_DIR"
    exit 1
fi

cp "$HEX_FILE" "$BIN_DIR/claycast-controller.hex"
echo "Saved controller build to $BIN_DIR/claycast-controller.hex"

rm -rf "$TEMP_DIR"

echo "All builds completed."
