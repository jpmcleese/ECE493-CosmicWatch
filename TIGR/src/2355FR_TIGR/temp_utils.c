// temp_utils.c
// Temperature sensor implementation for TIGR project
// Adapted for MSP430FR2355 internal temperature sensor with ADC
//
// Key differences from FR6989:
// - Uses ADC (10/12-bit) instead of ADC12
// - Temperature sensor is on channel A12 (not A30)
// - Reference setup uses PMMCTL2 instead of REFCTL0
// - Different register names: ADCCTL0, ADCCTL1, ADCMCTL0, ADCMEM0

#include "temp_utils.h"

// Temperature calibration addresses for FR2355 (from TLV)
// These are for 1.5V reference at 30°C and 85°C
#define CALADC_15V_30C  *((unsigned int *)0x1A1A)
#define CALADC_15V_85C  *((unsigned int *)0x1A1C)

// Initialize ADC for temperature sensing on MSP430FR2355
void adc_init(void) {
    // Configure reference and temperature sensor via PMM
    // Unlock PMM registers
    PMMCTL0_H = PMMPW_H;
    
    // Enable internal reference (1.5V) and temperature sensor
    PMMCTL2 |= INTREFEN | TSENSOREN;
    
    // Wait for reference to settle (minimum 400us)
    __delay_cycles(8000);                      // ~1ms at 8MHz
    
    // Configure ADC
    // ADCSHT_8 = 256 ADCCLK cycles sample time (temperature sensor needs >30us)
    // ADCON = Turn on ADC
    ADCCTL0 = ADCSHT_8 | ADCON;
    
    // ADCSHP = Use sampling timer (pulse mode)
    // Default: ADCCONSEQ = 0 (single-channel single-conversion)
    // Default: ADCSSEL = MODCLK
    ADCCTL1 = ADCSHP;
    
    // Clear resolution bits and set to 12-bit
    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;                       // 12-bit resolution
    
    // Configure input channel
    // ADCSREF_1 = VR+ = VREF, VR- = AVSS
    // ADCINCH_12 = Channel A12 (internal temperature sensor on FR2355)
    ADCMCTL0 = ADCSREF_1 | ADCINCH_12;
    
    // Disable ADC interrupt (we'll poll)
    ADCIE = 0x0000;
    
    // Additional settling time
    __delay_cycles(8000);
}

// Read temperature from internal sensor
// Returns temperature in degrees Celsius
int read_temperature(void) {
    unsigned int adc_value;
    long temp;
    
    // Ensure reference and temp sensor are enabled
    // (in case they were disabled for power saving)
    PMMCTL0_H = PMMPW_H;
    if(!(PMMCTL2 & INTREFEN)) {
        PMMCTL2 |= INTREFEN | TSENSOREN;
        __delay_cycles(400);                   // Wait for reference to settle
    }
    
    // Enable conversions
    ADCCTL0 |= ADCENC;
    
    // Start conversion
    ADCCTL0 |= ADCSC;
    
    // Wait for conversion to complete
    while(ADCCTL1 & ADCBUSY);
    
    // Read result
    adc_value = ADCMEM0;
    
    // Disable conversions
    ADCCTL0 &= ~ADCENC;
    
    // Get calibration values from TLV
    // These are factory-calibrated values for 30°C and 85°C at 1.5V reference
    unsigned int cal_30 = CALADC_15V_30C;
    unsigned int cal_85 = CALADC_15V_85C;
    
    // Verify calibration data is valid (not erased flash = 0xFFFF)
    if (cal_30 == 0xFFFF || cal_85 == 0xFFFF || cal_85 == cal_30) {
        // No valid calibration data, return error value
        return -273;  // Absolute zero as error indicator
    }
    
    // Calculate temperature using two-point calibration
    // Temp = ((ADC - CAL30) * (85 - 30)) / (CAL85 - CAL30) + 30
    temp = (long)adc_value - (long)cal_30;
    temp = temp * 55;  // (85 - 30)
    temp = temp / ((long)cal_85 - (long)cal_30);
    temp = temp + 30;
    
    return (int)temp;
}

// Optional: Power down temperature sensor and reference to save power
void adc_power_down(void) {
    // Disable ADC
    ADCCTL0 &= ~(ADCENC | ADCON);
    
    // Disable reference and temperature sensor
    PMMCTL0_H = PMMPW_H;
    PMMCTL2 &= ~(INTREFEN | TSENSOREN);
}

// Optional: Read raw ADC value (for debugging)
unsigned int read_raw_adc(void) {
    // Ensure reference and temp sensor are enabled
    PMMCTL0_H = PMMPW_H;
    if(!(PMMCTL2 & INTREFEN)) {
        PMMCTL2 |= INTREFEN | TSENSOREN;
        __delay_cycles(400);
    }
    
    // Enable and start conversion
    ADCCTL0 |= ADCENC | ADCSC;
    
    // Wait for conversion
    while(ADCCTL1 & ADCBUSY);
    
    // Read and return result
    unsigned int result = ADCMEM0;
    
    // Disable conversions
    ADCCTL0 &= ~ADCENC;
    
    return result;
}
