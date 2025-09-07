#!/bin/bash

# ESP32 Data Logger Build Script
# This script helps set up the ESP-IDF environment and build the project

set -e

echo "ESP32 Data Logger Build Script"
echo "=============================="

# Check if ESP-IDF is installed
if [ -z "$IDF_PATH" ]; then
    echo "ESP-IDF environment not set up."
    echo "Please install ESP-IDF and run:"
    echo "  . \$HOME/esp/esp-idf/export.sh"
    echo ""
    echo "Or if ESP-IDF is installed elsewhere, run:"
    echo "  . /path/to/esp-idf/export.sh"
    echo ""
    echo "For ESP-IDF installation instructions, visit:"
    echo "  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/"
    exit 1
fi

echo "ESP-IDF found at: $IDF_PATH"
echo "ESP-IDF version: $(idf.py --version)"
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "main" ]; then
    echo "Error: Please run this script from the project root directory"
    echo "Expected files: CMakeLists.txt, main/ directory"
    exit 1
fi

echo "Project structure verified."
echo ""

# Clean previous build
echo "Cleaning previous build..."
idf.py fullclean

# Configure project
echo "Configuring project..."
idf.py reconfigure

# Build project
echo "Building project..."
idf.py build

echo ""
echo "Build completed successfully!"
echo ""
echo "To flash the firmware to your ESP32:"
echo "  idf.py -p /dev/ttyUSB0 flash monitor"
echo ""
echo "Replace /dev/ttyUSB0 with your ESP32's serial port"
echo ""
echo "Common serial ports:"
echo "  Linux: /dev/ttyUSB0, /dev/ttyACM0"
echo "  macOS: /dev/cu.usbserial-*, /dev/cu.usbmodem-*"
echo "  Windows: COM1, COM2, etc."
echo ""
echo "After flashing, connect to WiFi 'ESP32-LOGGER' and visit:"
echo "  http://10.10.10.10"