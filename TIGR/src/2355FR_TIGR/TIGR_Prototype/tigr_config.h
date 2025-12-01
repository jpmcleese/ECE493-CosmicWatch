// tigr_config.h
// Common configuration and definitions for TIGR project
// Adapted for MSP430FR2355

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

// Software RTC Variables (MSP430FR2355 doesn't have hardware RTC_C)
// These replace the hardware RTCYEAR, RTCMON, etc. registers
extern volatile unsigned int rtc_year;      // Year (e.g., 2025)
extern volatile unsigned char rtc_month;    // Month (1-12)
extern volatile unsigned char rtc_day;      // Day (1-31)
extern volatile unsigned char rtc_hour;     // Hour (0-23)
extern volatile unsigned char rtc_minute;   // Minute (0-59)
extern volatile unsigned char rtc_second;   // Second (0-59)
extern volatile unsigned int rtc_ms;        // Milliseconds counter (0-999)

// Software RTC Macros for compatibility with existing code
// These allow sd_utils.c to use the same variable names
#define RTCYEAR   rtc_year
#define RTCMON    rtc_month
#define RTCDAY    rtc_day
#define RTCHOUR   rtc_hour
#define RTCMIN    rtc_minute
#define RTCSEC    rtc_second

// Function to initialize software RTC timer
// Note: Set rtc_year, rtc_month, etc. globals before calling this
void rtc_init(void);

#endif /* _TIGR_CONFIG_H */