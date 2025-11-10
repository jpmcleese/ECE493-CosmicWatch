// sd_utils.h
// SD Card data logging interface for TIGR project
// Handles higher-level data formatting and storage

#ifndef _TIGR_SD_H
#define _TIGR_SD_H

#include <msp430.h>
#include "tigr_config.h"

// Function prototypes
void save_reading(unsigned char band);
void write_readings_to_sd(void);
void flush_buffer_to_sd(void);
void sd_card_init(void);
void display_buffer_contents(void);  // Debug function

#endif /* _TIGR_SD_H */
