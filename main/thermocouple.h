#pragma once
#include <stdint.h>

// MAX31855 SPI interface
#define MAX31855_CS_0    GPIO_NUM_15  // CS for thermocouple 0
#define MAX31855_CS_1    GPIO_NUM_2   // CS for thermocouple 1  
#define MAX31855_CS_2    GPIO_NUM_4   // CS for thermocouple 2
#define MAX31855_CS_3    GPIO_NUM_16  // CS for thermocouple 3
#define MAX31855_MOSI    GPIO_NUM_23  // Shared MOSI
#define MAX31855_MISO    GPIO_NUM_19  // Shared MISO
#define MAX31855_CLK     GPIO_NUM_18  // Shared CLK

// Function declarations
void thermocouple_init(void);
float thermocouple_read(int channel);
int thermocouple_get_fault(int channel);
