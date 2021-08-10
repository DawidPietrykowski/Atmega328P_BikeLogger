#pragma once

#define WDT_AVG_16MS_CYCLE 16.55f
#define WDT_AVG_64MS_CYCLE 66.2f

#define SERIAL_BAUD 9600

#define ENTRY_COUNT 500 // entry buffer size

#define MEASUREMENT_DURATION 4000   // time between entries
#define ENTRIES_UNTIL_BATT_CHECK 4 // check battery every x entries
// #define MEASUREMENT_DURATION 500   // time between entries
// #define ENTRIES_UNTIL_BATT_CHECK 1 // check battery every x entries

#define LED_PIN 4       // output pin for the LED 
#define BUZZER_PIN 5    // output pin for the buzzer
#define MAGNET_INPUT_PIN 3    // input pin for the magnet sensor
#define SAVE_INPUT_PIN 2    // input pin for the magnet sensor
#define VREF_PIN 7    // input pin for the magnet sensor
#define SD_CS_PIN 10    // output pin for the SD Chip Select

#define FILE_BASE_NAME "DataSession"

// set to false in normal operation
// true for debug over serial
#define DEBUG true
#define DEBUG_SERIAL if(DEBUG)Serial
