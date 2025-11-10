// sd_utils.c
// SD Card data logging implementation for TIGR project

#include <string.h>
#include "sd_utils.h"
#include "tigr_config.h"
#include "tigr_mmc.h"
#include "tigr_utils.h"
#include "temp_utils.h"
#include "UART.h"

// Display buffer contents to UART (for debugging)
void display_buffer_contents(void) {
    unsigned int i;
    
    UART1string("\r\n========== SD BUFFER CONTENTS ==========\r\n");
    UART1string("Sector: ");
    char sector_str[12];
    uint_to_string((unsigned int)current_sector, sector_str);
    UART1string((unsigned char*)sector_str);
    UART1string("\r\n");
    UART1string("Buffer Position: ");
    char pos_str[12];
    uint_to_string(buffer_position, pos_str);
    UART1string((unsigned char*)pos_str);
    UART1string(" bytes\r\n");
    UART1string("--------------------------------------------\r\n");
    
    // Display the actual data (stop at first null or end of buffer)
    for (i = 0; i < SD_BUFFER_SIZE && i < buffer_position; i++) {
        if (sd_buffer[i] == 0) break;
        UART1send(sd_buffer[i]);
    }
    
    UART1string("\r\n========================================\r\n\r\n");
}

// Function to save current reading
void save_reading(unsigned char band) {
        readings[reading_count].energy_band = band;
        readings[reading_count].muon_number = muon_count;
        
        // Read current RTC values (they are in BCD format)
        readings[reading_count].year = RTCYEAR;
        readings[reading_count].month = RTCMON;
        readings[reading_count].day = RTCDAY;
        readings[reading_count].hour = RTCHOUR;
        readings[reading_count].minute = RTCMIN;
        readings[reading_count].second = RTCSEC;
        
        // Read temperature
        readings[reading_count].temperature = read_temperature();
        
        // Display on UART
        UART1string("Reading saved: Band ");
        UART1send('0' + band);
        UART1string(", Muon #");
        char time_str[3];
        char num_str[12];
        uint_to_string(muon_count, num_str);
        UART1string((unsigned char*)num_str);
        UART1string(", Time ");
        bcd_to_string(readings[reading_count].hour, time_str);
        UART1string((unsigned char*)time_str);
        UART1string(":");
        bcd_to_string(readings[reading_count].minute, time_str);
        UART1string((unsigned char*)time_str);
        UART1string(":");
        bcd_to_string((unsigned char)readings[reading_count].second, time_str);
        UART1string((unsigned char*)time_str);
        UART1string("\r\n\n");
        reading_count++;
    }

// Write readings to SD card
void write_readings_to_sd(void) {
    unsigned int i, j;
    char muon_str[12], band_str[4], temp_str[12];
    char year_str[6], month_str[4], day_str[4];
    char hour_str[4], min_str[4], sec_str[4];
    
    if (!sd_initialized) {
        UART1string("\r\n*** SD NOT INITIALIZED ***\r\n");
        UART1string("Showing what WOULD be written to SD:\r\n\r\n");
    }
    
    UART1string("Preparing to write ");
    char count_str[12];
    uint_to_string(reading_count, count_str);
    UART1string((unsigned char*)count_str);
    UART1string(" readings...\r\n");
    
    // Format each reading and add to buffer
    for (i = 0; i < reading_count; i++) {
        // Convert all fields to strings
        uint_to_string(readings[i].muon_number, muon_str);
        uint_to_string(readings[i].energy_band, band_str);
        hex_to_string_4(readings[i].year, year_str);
        bcd_to_string(readings[i].month, month_str);
        bcd_to_string(readings[i].day, day_str);
        bcd_to_string(readings[i].hour, hour_str);
        bcd_to_string(readings[i].minute, min_str);
        bcd_to_string((unsigned char)readings[i].second, sec_str);
        int_to_string(readings[i].temperature, temp_str);
        
        
        // Build the CSV line manually and add to buffer
        // Format: "Muon#,Band,YYYY-MM-DD,HH:MM:SS,Temperature\n"
        
        // Add muon number
        for (j = 0; muon_str[j] != '\0'; j++) {
            sd_buffer[buffer_position++] = muon_str[j];
        }
        sd_buffer[buffer_position++] = ',';
        
        // Add band
        for (j = 0; band_str[j] != '\0'; j++) {
            sd_buffer[buffer_position++] = band_str[j];
        }
        sd_buffer[buffer_position++] = ',';
        
        // Add date: YYYY-MM-DD
        for (j = 0; j < 4; j++) {
            sd_buffer[buffer_position++] = year_str[j];
        }
        sd_buffer[buffer_position++] = '-';
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = month_str[j];
        }
        sd_buffer[buffer_position++] = '-';
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = day_str[j];
        }
        sd_buffer[buffer_position++] = ',';
        
        // Add time: HH:MM:SS
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = hour_str[j];
        }
        sd_buffer[buffer_position++] = ':';
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = min_str[j];
        }
        sd_buffer[buffer_position++] = ':';
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = sec_str[j];
        }
        sd_buffer[buffer_position++] = ',';
        
        // Add temperature
        for (j = 0; temp_str[j] != '\0'; j++) {
            sd_buffer[buffer_position++] = temp_str[j];
        }
        sd_buffer[buffer_position++] = '\n';
        
        // Check if buffer would overflow (leaving room for more data)
        if (buffer_position >= SD_BUFFER_SIZE - 64) {
            UART1string("Buffer full, flushing sector...\r\n");
            flush_buffer_to_sd();
        }
    }
    
    // Flush any remaining data
    if (buffer_position > 0) {
        UART1string("Flushing remaining data...\r\n");
        flush_buffer_to_sd();
    }
    
    UART1string("Write complete!\r\n\r\n");
}

// Flush buffer to SD card
void flush_buffer_to_sd(void) {
    if (buffer_position == 0) return;
    
    // ALWAYS display buffer contents for debugging
    display_buffer_contents();
    
    // Fill rest of buffer with zeros
    while (buffer_position < SD_BUFFER_SIZE) {
        sd_buffer[buffer_position++] = 0;
    }
    
    // Write buffer to SD card (if initialized)
    if (sd_initialized) {
        if (mmc_write_sector(current_sector, sd_buffer) == MMC_SUCCESS) {
            UART1string(">>> SUCCESS: Data written to sector ");
            char sector_str[12];
            uint_to_string((unsigned int)current_sector, sector_str);
            UART1string((unsigned char*)sector_str);
            UART1string("\r\n\r\n");
            current_sector++;  // Move to next sector
        } else {
            UART1string(">>> ERROR: SD write failed!\r\n\r\n");
        }
    } else {
        UART1string(">>> SD NOT AVAILABLE: Data shown above (not written)\r\n\r\n");
    }
    
    // Reset buffer
    buffer_position = 0;
    memset(sd_buffer, 0, SD_BUFFER_SIZE);
}

// Initialize SD card
void sd_card_init(void) {
    unsigned char retry_count = 0;
    
    UART1string("\r\n========= SD Card Initialization ========\r\n");
    UART1string("Checking for card presence...\r\n");
    
    // Wait for card to be inserted
    while (!mmc_ping() && retry_count < 30) {
        __delay_cycles(1000000); // Wait 1 second
        retry_count++;
        if (retry_count % 3 == 0) {
            UART1string("Waiting for SD card...\r\n");
        }
    }
    
    if (retry_count >= 10) {
        // No card detected
        UART1string("ERROR: No SD card detected!\r\n");
        UART1string("*** DEBUG MODE: Will show buffer contents instead ***\r\n");
        UART1string("========================================\r\n\r\n");
        sd_initialized = 0;
        return;
    }
    
    UART1string("Card detected! Initializing...\r\n");
    
    // Initialize the card
    retry_count = 0;
    while (mmc_init() != MMC_SUCCESS && retry_count < 3) {
        __delay_cycles(1000000); // Wait 1 second
        retry_count++;
        UART1string("Init attempt ");
        UART1send('0' + retry_count);
        UART1string(" of 3...\r\n");
    }
    
    if (retry_count >= 3) {
        UART1string("ERROR: SD card initialization failed!\r\n");
        UART1string("*** DEBUG MODE: Will show buffer contents instead ***\r\n");
        UART1string("========================================\r\n\r\n");
        sd_initialized = 0;
    } else {
        UART1string("SUCCESS: SD card initialized!\r\n");
        UART1string("Card is ready for data logging\r\n");
        sd_initialized = 1;
        // Clear the buffer
        memset(sd_buffer, 0, SD_BUFFER_SIZE);
        __delay_cycles(1000000); // Wait 1 second
        UART1string("Ready to log data!\r\n");
        UART1string("========================================\r\n\r\n");
    }
}
