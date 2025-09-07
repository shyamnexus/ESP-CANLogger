# ESP32 Data Logger

A comprehensive data logging system for ESP32 with 4-channel K-type thermocouple support, GPS tracking, CAN bus monitoring, and SD card storage.

## Features

- **4-Channel K-Type Thermocouple Interface**: MAX31855 SPI-based temperature sensors
- **GPS Tracking**: Neo-M8N GPS module with 10Hz update rate
- **CAN Bus Monitoring**: Configurable CAN bus speeds (125, 250, 500, 1000 kbps)
- **SD Card Logging**: CSV format data logging with timestamp
- **OLED Display**: Real-time data display on SSD1306 OLED
- **Web Interface**: WiFi AP with web-based configuration and monitoring
- **12V Car Battery Support**: Designed for automotive applications

## Hardware Requirements

### ESP32 OLED Module (Robu.in)
- ESP32-WROOM-32 module
- Built-in OLED display (SSD1306)
- WiFi and Bluetooth support

### Additional Components
- 4x MAX31855 thermocouple amplifiers
- 4x K-type thermocouples
- Neo-M8N GPS module
- SD card module
- CAN transceiver (MCP2515 or similar)
- 12V to 5V/3.3V power supply

## Pin Connections

### MAX31855 Thermocouple Amplifiers (SPI)
- MOSI: GPIO 23
- MISO: GPIO 19
- CLK: GPIO 18
- CS0: GPIO 15 (Thermocouple 1)
- CS1: GPIO 2  (Thermocouple 2)
- CS2: GPIO 4  (Thermocouple 3)
- CS3: GPIO 16 (Thermocouple 4)

### Neo-M8N GPS (UART2)
- TX: GPIO 17
- RX: GPIO 16
- Baud Rate: 38400

### SSD1306 OLED (I2C)
- SDA: GPIO 21
- SCL: GPIO 22
- Address: 0x3C

### SD Card (SPI)
- MOSI: GPIO 23 (shared with thermocouples)
- MISO: GPIO 19 (shared with thermocouples)
- CLK: GPIO 18 (shared with thermocouples)
- CS: GPIO 5

### CAN Bus (TWAI)
- TX: GPIO 21 (shared with OLED SDA)
- RX: GPIO 22 (shared with OLED SCL)

### Power Monitoring
- Battery Voltage: GPIO 34 (ADC input)

## Software Setup

### Prerequisites
1. Install ESP-IDF v5.0 or later
2. Set up ESP-IDF environment:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

### Building the Project
1. Clone or download this project
2. Navigate to the project directory
3. Configure the project:
   ```bash
   idf.py menuconfig
   ```
4. Build the project:
   ```bash
   idf.py build
   ```
5. Flash to ESP32:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

### Configuration Options
- Log rate: 1-100 Hz (default: 10 Hz)
- CAN bus speed: 125, 250, 500, 1000 kbps (default: 500 kbps)
- WiFi AP: "ESP32-LOGGER" (IP: 10.10.10.10)

## Usage

### Initial Setup
1. Power on the device with 12V supply
2. Connect to WiFi network "ESP32-LOGGER"
3. Open web browser to http://10.10.10.10
4. Configure logging parameters as needed

### Web Interface
- **Configuration**: Set log rate and CAN bus speed
- **Live Data**: Monitor real-time sensor readings
- **Data Files**: Download CSV log files
- **System Status**: View system information

### Data Logging
- Data is automatically logged to SD card in CSV format
- Files are named with timestamp: `log_YYYYMMDD_HHMMSS.csv`
- CSV format includes: Timestamp, T1-T4, GPS_Fix, Latitude, Longitude, CAN_ID, CAN_Data

### OLED Display
- Real-time temperature readings (T1-T4)
- GPS status and coordinates
- CAN bus activity
- System status messages

## File Structure

```
├── main/
│   ├── main.c              # Main application entry point
│   ├── logger.c/h          # Data logging functionality
│   ├── thermocouple.c/h    # MAX31855 interface
│   ├── gps.c/h             # Neo-M8N GPS interface
│   ├── oled.c/h            # SSD1306 OLED display
│   ├── canbus.c/h          # CAN bus interface
│   └── wifi_config.c/h     # WiFi AP and web server
├── spiffs/
│   ├── index.html          # Web interface
│   └── style.css           # Web interface styling
├── partitions.csv          # Partition table
├── sdkconfig.defaults      # Default configuration
└── CMakeLists.txt          # Build configuration
```

## Troubleshooting

### Common Issues

1. **SD Card Not Detected**
   - Check SD card connections
   - Ensure SD card is formatted (FAT32)
   - Verify SPI pin connections

2. **GPS No Fix**
   - Ensure GPS antenna is connected
   - Check UART connections
   - Wait for GPS to acquire satellites (can take several minutes)

3. **Thermocouple Readings Invalid**
   - Check thermocouple connections
   - Verify MAX31855 wiring
   - Check for thermocouple faults

4. **CAN Bus Not Working**
   - Verify CAN transceiver connections
   - Check CAN bus termination resistors
   - Ensure correct baud rate configuration

5. **OLED Display Issues**
   - Check I2C connections
   - Verify OLED address (0x3C)
   - Check power supply

### Debug Information
- Monitor serial output at 115200 baud for debug messages
- Check web interface system status
- Use ESP-IDF monitor for detailed logging

## Power Consumption

- Typical current draw: ~200mA at 5V
- Maximum current: ~500mA (during SD card writes)
- 12V input with 5V/3.3V regulation required
- Suitable for car battery operation

## License

This project is open source. Please check individual component licenses for commercial use.

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review ESP-IDF documentation
3. Check component datasheets
4. Verify hardware connections

## Version History

- v1.0: Initial release with basic functionality
- v1.1: Added web interface and improved error handling
- v1.2: Enhanced GPS configuration and OLED display