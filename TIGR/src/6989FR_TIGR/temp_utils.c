// temp_utils.c
// Temperature sensor implementation for TIGR project
// Uses MSP430FR6989 internal temperature sensor with ADC12

#include "temp_utils.h"

// Initialize ADC for temperature sensing
void adc_init(void) {
    // Configure REF module first
    while(REFCTL0 & REFGENBUSY);               // Wait if reference generator is busy
    REFCTL0 = REFVSEL_0 | REFON;               // Enable internal 1.2V reference
    
    // Wait for reference to settle (minimum 75us at 1MHz = 75 cycles)
    __delay_cycles(8000);
    
    // Configure ADC12 - Use longest sampling time for temp sensor
    ADC12CTL0 = ADC12SHT0_15 | ADC12ON;        // 512 ADC12CLK cycles sampling, ADC12 on
    ADC12CTL1 = ADC12SHP;                      // Use sampling timer, MODCLK source
    ADC12CTL2 = ADC12RES_2;                    // 12-bit resolution
    ADC12CTL3 = ADC12TCMAP;                    // Enable internal temperature sensor
    ADC12MCTL0 = ADC12VRSEL_1 | ADC12INCH_30;  // VR+ = VREF, VR- = AVSS, Channel A30 (temp sensor)
    ADC12IER0 = 0x0000;                        // Disable interrupts
    
    // Additional settling time for temperature sensor
    __delay_cycles(8000);
}

// Read temperature from internal sensor
int read_temperature(void) {
    unsigned int adc_value;
    long temp;
    
    // Wait for reference to be ready
    while(!(REFCTL0 & REFGENRDY));
    
    // Enable conversions
    ADC12CTL0 |= ADC12ENC;
    
    // Start conversion
    ADC12CTL0 |= ADC12SC;
    
    // Wait for conversion to complete
    while (ADC12CTL1 & ADC12BUSY);
    
    // Read result
    adc_value = ADC12MEM0;
    
    // Disable conversions
    ADC12CTL0 &= ~ADC12ENC;
    
    // Get calibration values from TLV for FR6989
    // These are factory-calibrated values for 30°C and 85°C at 1.2V ref
    unsigned int cal_30 = *((unsigned int *)0x1A1A);   // CAL_ADC_12T30
    unsigned int cal_85 = *((unsigned int *)0x1A1C);   // CAL_ADC_12T85
    
    // Verify calibration data is valid (not erased flash = 0xFFFF)
    if (cal_30 == 0xFFFF || cal_85 == 0xFFFF || cal_85 == cal_30) {
        // No valid calibration data, return error value
        return -273;  // Absolute zero as error indicator
    }
    
    // Calculate temperature using calibration
    // Temp = ((ADC - CAL30) * (85 - 30)) / (CAL85 - CAL30) + 30
    temp = (long)adc_value - (long)cal_30;
    temp = temp * 55;  // (85 - 30)
    temp = temp / ((long)cal_85 - (long)cal_30);
    temp = temp + 30;
    
    return (int)temp;
}
