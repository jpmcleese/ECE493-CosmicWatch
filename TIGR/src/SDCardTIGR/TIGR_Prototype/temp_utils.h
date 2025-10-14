// temp_utils.h
// Temperature sensor interface for TIGR project
// Uses MSP430FR6989 internal temperature sensor

#ifndef _TIGR_TEMP_H
#define _TIGR_TEMP_H

#include <msp430.h>

// Function prototypes
void adc_init(void);
int read_temperature(void);

#endif /* _TIGR_TEMP_H */
