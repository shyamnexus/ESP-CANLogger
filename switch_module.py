#!/usr/bin/env python3
"""
Module switching utility for ESP32 Data Logger
This script helps you easily switch between different modules for testing
"""

import os
import sys

# Available modules
MODULES = {
    'oled': 'ENABLE_OLED_MODULE',
    'thermocouple': 'ENABLE_THERMOCOUPLE_MODULE', 
    'gps': 'ENABLE_GPS_MODULE',
    'canbus': 'ENABLE_CANBUS_MODULE',
    'wifi': 'ENABLE_WIFI_CONFIG_MODULE',
    'logger': 'ENABLE_LOGGER_MODULE',
    'sdcard': 'ENABLE_SDCARD_MODULE',
    'power': 'ENABLE_POWER_MONITORING_MODULE'
}

def get_current_module():
    """Get the currently enabled module"""
    config_file = "/workspace/main/module_config.h"
    
    if not os.path.exists(config_file):
        return None
    
    with open(config_file, 'r') as f:
        content = f.read()
    
    for module_name, define in MODULES.items():
        # Check for uncommented define
        if f"#define {define}" in content and f"// #define {define}" not in content:
            return module_name
    
    return None

def switch_to_module(target_module):
    """Switch to the specified module"""
    if target_module not in MODULES:
        print(f"❌ Unknown module: {target_module}")
        print(f"Available modules: {', '.join(MODULES.keys())}")
        return False
    
    config_file = "/workspace/main/module_config.h"
    
    if not os.path.exists(config_file):
        print(f"❌ Configuration file not found: {config_file}")
        return False
    
    with open(config_file, 'r') as f:
        content = f.read()
    
    # Comment out all modules (handle both commented and uncommented states)
    for module_name, define in MODULES.items():
        # Remove any existing comments and add new ones
        content = content.replace(f"#define {define}", f"// #define {define}")
        content = content.replace(f"// // #define {define}", f"// #define {define}")
    
    # Enable the target module
    target_define = MODULES[target_module]
    content = content.replace(f"// #define {target_define}", f"#define {target_define}")
    
    # Write back to file
    with open(config_file, 'w') as f:
        f.write(content)
    
    print(f"✅ Switched to {target_module} module")
    return True

def list_modules():
    """List all available modules"""
    print("Available modules:")
    for module_name, define in MODULES.items():
        print(f"  - {module_name}: {define}")

def main():
    if len(sys.argv) < 2:
        current = get_current_module()
        if current:
            print(f"Current module: {current}")
        else:
            print("No module currently enabled")
        print()
        list_modules()
        print()
        print("Usage: python3 switch_module.py <module_name>")
        print("Example: python3 switch_module.py oled")
        return
    
    target_module = sys.argv[1].lower()
    
    if target_module == 'list':
        list_modules()
        return
    
    current = get_current_module()
    if current == target_module:
        print(f"✅ {target_module} module is already enabled")
        return
    
    if switch_to_module(target_module):
        print(f"🔄 Switched from {current or 'none'} to {target_module}")
        print("📝 Don't forget to rebuild the project!")

if __name__ == "__main__":
    main()