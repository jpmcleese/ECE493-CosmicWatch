// tigr_config.h
// Common configuration and definitions for TIGR project

#ifndef _TIGR_CONFIG_H
#define _TIGR_CONFIG_H

#include <msp430.h>

// Structure to store energy band readings with timestamp
typedef struct {
    unsigned int muon_number;    // Muon event number
    unsigned char energy_band;   // Energy band (1-4)
    unsigned int year;           // Year
    unsigned char month;         // Month (1-12)
    unsigned char day;           // Day (1-31)
    unsigned char hour;          // Hour (0-23)
    unsigned char minute;        // Minute (0-59)
    unsigned int second;         // Second (0-59)
    int temperature;             // Temperature in Celsius
} EnergyReading;

// Configuration Constants
#define MAX_READINGS 16           // Number of readings before SD write
#define SD_BUFFER_SIZE 512       // SD card sector size

// Global Variables (extern declarations)
extern EnergyReading readings[MAX_READINGS];
extern volatile unsigned int reading_count;
extern volatile unsigned int muon_count;
extern unsigned char sd_buffer[SD_BUFFER_SIZE];
extern volatile unsigned char sd_initialized;
extern unsigned long current_sector;
extern unsigned int buffer_position;

#endif /* _TIGR_CONFIG_H */
