#!/bin/bash

# Ensure arduino-cli is installed
if ! command -v arduino-cli &> /dev/null; then
    echo "arduino-cli is not installed. Please install it first."
    exit 1
fi

PACKAGE_NAME="arduino:avr"

# Update the index of available boards
echo "Updating board index..."
arduino-cli core update-index

# Install the AVR board package
echo "Installing AVR board package..."
arduino-cli core install "$PACKAGE_NAME"

echo "Arduino AVR board package has been installed!"