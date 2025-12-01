// temp_utils.h
// Temperature sensor interface for TIGR project
// Adapted for MSP430FR2355 internal temperature sensor

#ifndef _TIGR_TEMP_H
#define _TIGR_TEMP_H

#include <msp430.h>

// Function prototypes
void adc_init(void);
int read_temperature(void);
void adc_power_down(void);
unsigned int read_raw_adc(void);

#endif /* _TIGR_TEMP_H */
