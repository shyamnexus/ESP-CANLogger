# Hardware Connection Guide

## ESP32 OLED Module Pin Connections

### MAX31855 Thermocouple Amplifiers (4x)
```
ESP32 Pin    MAX31855 Pin    Function
GPIO 23      MOSI           SPI Data Out
GPIO 19      MISO           SPI Data In  
GPIO 18      CLK            SPI Clock
GPIO 15      CS             Chip Select (Thermocouple 1)
GPIO 2       CS             Chip Select (Thermocouple 2)
GPIO 4       CS             Chip Select (Thermocouple 3)
GPIO 16      CS             Chip Select (Thermocouple 4)
3.3V         VCC            Power Supply
GND          GND            Ground
```

### Neo-M8N GPS Module
```
ESP32 Pin    GPS Pin        Function
GPIO 17      TX             UART Transmit
GPIO 16      RX             UART Receive
3.3V         VCC            Power Supply
GND          GND            Ground
```

### SSD1306 OLED Display
```
ESP32 Pin    OLED Pin       Function
GPIO 21      SDA            I2C Data
GPIO 22      SCL            I2C Clock
3.3V         VCC            Power Supply
GND          GND            Ground
```

### SD Card Module
```
ESP32 Pin    SD Card Pin    Function
GPIO 23      MOSI           SPI Data Out (shared with thermocouples)
GPIO 19      MISO           SPI Data In (shared with thermocouples)
GPIO 18      CLK            SPI Clock (shared with thermocouples)
GPIO 5       CS             Chip Select
3.3V         VCC            Power Supply
GND          GND            Ground
```

### CAN Bus Transceiver (MCP2515)
```
ESP32 Pin    MCP2515 Pin    Function
GPIO 21      SI             SPI Data Out (shared with OLED SDA)
GPIO 22      SO             SPI Data In (shared with OLED SCL)
GPIO 18      SCK            SPI Clock (shared with thermocouples)
GPIO 21      CS             Chip Select
3.3V         VCC            Power Supply
GND          GND            Ground
```

### Power Monitoring
```
ESP32 Pin    Connection     Function
GPIO 34      Voltage Divider    Battery Voltage Monitor
```

## Power Supply Requirements

### 12V to 5V/3.3V Conversion
- Input: 12V from car battery
- Output: 5V for modules, 3.3V for ESP32
- Current: 500mA minimum
- Recommended: LM2596 or similar buck converter

### Voltage Divider for Battery Monitoring
```
12V Battery ----[10kΩ]----+----[2.2kΩ]---- GND
                          |
                       GPIO 34
```
This creates a 1:5.5 voltage divider for safe ADC reading.

## Component Specifications

### MAX31855 Thermocouple Amplifier
- Input: K-type thermocouple
- Temperature range: -200°C to +1350°C
- Accuracy: ±2°C
- SPI interface
- Cold junction compensation

### Neo-M8N GPS Module
- Update rate: 10Hz (configurable)
- Accuracy: 2.5m CEP
- Power consumption: 25mA
- UART interface: 38400 baud

### SSD1306 OLED Display
- Size: 128x64 pixels
- Interface: I2C
- Address: 0x3C
- Power consumption: 20mA

### SD Card Module
- Interface: SPI
- Supported formats: FAT32
- Capacity: 2GB to 32GB recommended
- Speed: Class 10 or higher

## Assembly Notes

1. **Power Supply**: Use a reliable 12V to 5V/3.3V converter with adequate current capacity
2. **Grounding**: Ensure all components share a common ground
3. **Shielding**: Consider shielding for GPS antenna and thermocouple wires
4. **Connectors**: Use appropriate connectors for automotive environment
5. **Enclosure**: Use weatherproof enclosure for outdoor applications

## Testing Procedure

1. **Power On**: Verify 3.3V and 5V supplies are correct
2. **Serial Monitor**: Check for initialization messages
3. **WiFi**: Connect to "ESP32-LOGGER" network
4. **Web Interface**: Access http://10.10.10.10
5. **Temperature**: Test thermocouple readings
6. **GPS**: Wait for satellite fix (may take several minutes)
7. **SD Card**: Verify file creation and logging
8. **CAN Bus**: Test with known CAN messages

## Troubleshooting

### No Temperature Readings
- Check thermocouple connections
- Verify MAX31855 power supply
- Check SPI connections
- Look for fault indicators in logs

### GPS No Fix
- Ensure antenna is connected
- Check UART connections
- Wait for satellite acquisition
- Verify GPS module power

### SD Card Issues
- Check card format (FAT32)
- Verify SPI connections
- Check card capacity and speed
- Ensure proper power supply

### OLED Display Problems
- Check I2C connections
- Verify OLED address
- Check power supply
- Look for I2C bus conflicts

### CAN Bus Not Working
- Check transceiver connections
- Verify termination resistors
- Check baud rate settings
- Ensure proper CAN bus wiring