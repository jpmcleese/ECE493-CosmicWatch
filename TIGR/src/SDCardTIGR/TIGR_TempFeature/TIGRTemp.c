//  _______ _____ _____ _____   _______ 
// |__   __|_   _/ ____|  __ \ |__   __|
//    | |    | || |  __| |__) |   | |   
//    | |    | || | |_ |  _  /    | |   
//    | |   _| || |__| | | \ \    | |   
//    |_|  |_____\_____|_|  \_\   |_|   
//
//  Tiny Instrument for Gathering Radiation and Temperature (TIGR)
//  Version: 2.3 + Temperature Sensor Integration
//  Date: 10/9/2025
//
//  Authors:
//    - Kevin Nguyen
//    - Daniel Uribe
//    - Arda Cobanoglu
//    - JP McLeese III
//
//  Description:
//    This version of TIGR extends Version 2.3 with an onboard temperature sensor
//    using the MSP430’s internal ADC. A separate file was created for this feature,
//    since the ADC was initially avoided to minimize power consumption on
//    coin-cell-powered systems.
//
//  Changelog:
//    - Added UART debug messages showing what would be written to the SD card
//      (to be removed in the final release).
//
//    - Integrated MSP430 internal temperature sensor to log on-chip temperature (°C)
//      at each interrupt event.
//
//    - Fixed buffer issue that prevented correct muon event storage.
//
//    - Corrected timer initialization to start from 10/9/2025, 12:00 PM.
//
//    - Note: SD card data is written in raw sector format — a hex editor
//      is required to view contents since no FAT filesystem is used
//      (excluded to save power and memory).
//
//  Power Impact Note (From Frank):
//    - The MSP430 internal temperature sensor and ADC draw power only while active.
//    - During conversion, the ADC + reference consume ≈200–250 µA for ~1–2 ms.
//      → Sampling once per second ≈ 0.25 µA average current (minimal impact).
//      → Sampling once per minute or on rare events ≈ 0.004 µA (negligible).
//      → Continuous operation ≈ 250 µA (significant drain; not recommended).
//    - Best practice: enable ADC only when needed (e.g., at muon interrupts),
//      then power it down immediately after conversion. (Changes must be implemented to code)


#include "tigr_config.h"
#include "tigr_mmc.h"
#include "UART.h"

// Global Variables
EnergyReading readings[MAX_READINGS];
volatile unsigned int reading_count = 0;
volatile unsigned int muon_count = 0;

// SD Card variables
unsigned char sd_buffer[SD_BUFFER_SIZE];
unsigned long current_sector = 0;
unsigned int buffer_position = 0;
volatile unsigned char sd_initialized = 0;

// Helper function to convert unsigned int to string
void uint_to_string(unsigned int num, char* str) {
    int i = 0;
    int j;
    char temp;
    
    // Handle 0 case
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    // Convert number to string (reversed)
    while (num > 0) {
        str[i++] = '0' + (num % 10);
        num /= 10;
    }
    str[i] = '\0';
    
    // Reverse the string
    for (j = 0; j < i/2; j++) {
        temp = str[j];
        str[j] = str[i-1-j];
        str[i-1-j] = temp;
    }
}

// Helper function to convert signed int to string (for temperature)
void int_to_string(int num, char* str) {
    int i = 0;
    int j;
    char temp;
    int is_negative = 0;
    
    // Handle negative numbers
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    // Handle 0 case
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    // Convert number to string (reversed)
    while (num > 0) {
        str[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Add minus sign if negative
    if (is_negative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    
    // Reverse the string
    for (j = 0; j < i/2; j++) {
        temp = str[j];
        str[j] = str[i-1-j];
        str[i-1-j] = temp;
    }
}

// Helper to convert 2-digit BCD to string
void bcd_to_string(unsigned char bcd, char* str) {
    str[0] = '0' + ((bcd >> 4) & 0x0F);  // High nibble
    str[1] = '0' + (bcd & 0x0F);         // Low nibble
    str[2] = '\0';
}

// Helper to convert 4-digit hex to string (for year)
void hex_to_string_4(unsigned int hex, char* str) {
    str[0] = '0' + ((hex >> 12) & 0x0F);
    str[1] = '0' + ((hex >> 8) & 0x0F);
    str[2] = '0' + ((hex >> 4) & 0x0F);
    str[3] = '0' + (hex & 0x0F);
    str[4] = '\0';
}

// Initialize ADC for temperature sensing
void adc_init(void) {
    // Configure REF module first
    while(REFCTL0 & REFGENBUSY);               // Wait if reference generator is busy
    REFCTL0 = REFVSEL_0 | REFON;               // Enable internal 1.2V reference
    
    // Wait for reference to settle (minimum 75us at 1MHz = 75 cycles)
    __delay_cycles(8000);                      // 8ms to be safe
    
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

// Display buffer contents to UART (for debugging without SD card)
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
    if (reading_count < MAX_READINGS) {
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
        
        // DEBUG: Show raw RTC values and temperature
        UART1string("  [RTC Debug] Year=0x");
        char debug_str[6];
        hex_to_string_4(readings[reading_count].year, debug_str);
        UART1string((unsigned char*)debug_str);
        UART1string(" Mon=0x");
        bcd_to_string(readings[reading_count].month, debug_str);
        UART1string((unsigned char*)debug_str);
        UART1string(" Day=0x");
        bcd_to_string(readings[reading_count].day, debug_str);
        UART1string((unsigned char*)debug_str);
        UART1string(" Time=");
        bcd_to_string(readings[reading_count].hour, debug_str);
        UART1string((unsigned char*)debug_str);
        UART1string(":");
        bcd_to_string(readings[reading_count].minute, debug_str);
        UART1string((unsigned char*)debug_str);
        UART1string(":");
        bcd_to_string((unsigned char)readings[reading_count].second, debug_str);
        UART1string((unsigned char*)debug_str);
        UART1string(" Temp=");
        char temp_str[12];
        int_to_string(readings[reading_count].temperature, temp_str);
        UART1string((unsigned char*)temp_str);
        UART1string("C\r\n");
        
        reading_count++;
        
        // Display on UART
        UART1string("Reading saved: Band ");
        UART1send('0' + band);
        UART1string(", Muon #");
        char num_str[12];
        uint_to_string(muon_count, num_str);
        UART1string((unsigned char*)num_str);
        UART1string("\r\n");
    }
    else {
        // Array is full - save to SD card and reset
        UART1string("Buffer full, writing to SD...\r\n");
        write_readings_to_sd();
        reading_count = 0;
    }
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
    UART1string(" readings:\r\n");
    
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
        
        // Display row header
        UART1string("  Row ");
        char row_str[12];
        uint_to_string(i+1, row_str);
        UART1string((unsigned char*)row_str);
        UART1string(": ");
        
        // Build the CSV line manually and add to buffer
        // Format: "Muon#,Band,YYYY-MM-DD,HH:MM:SS,Temperature\n"
        
        // Add muon number
        for (j = 0; muon_str[j] != '\0'; j++) {
            sd_buffer[buffer_position++] = muon_str[j];
            UART1send(muon_str[j]);
        }
        sd_buffer[buffer_position++] = ',';
        UART1send(',');
        
        // Add band
        for (j = 0; band_str[j] != '\0'; j++) {
            sd_buffer[buffer_position++] = band_str[j];
            UART1send(band_str[j]);
        }
        sd_buffer[buffer_position++] = ',';
        UART1send(',');
        
        // Add date: YYYY-MM-DD
        for (j = 0; j < 4; j++) {
            sd_buffer[buffer_position++] = year_str[j];
            UART1send(year_str[j]);
        }
        sd_buffer[buffer_position++] = '-';
        UART1send('-');
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = month_str[j];
            UART1send(month_str[j]);
        }
        sd_buffer[buffer_position++] = '-';
        UART1send('-');
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = day_str[j];
            UART1send(day_str[j]);
        }
        sd_buffer[buffer_position++] = ',';
        UART1send(',');
        
        // Add time: HH:MM:SS
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = hour_str[j];
            UART1send(hour_str[j]);
        }
        sd_buffer[buffer_position++] = ':';
        UART1send(':');
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = min_str[j];
            UART1send(min_str[j]);
        }
        sd_buffer[buffer_position++] = ':';
        UART1send(':');
        for (j = 0; j < 2; j++) {
            sd_buffer[buffer_position++] = sec_str[j];
            UART1send(sec_str[j]);
        }
        sd_buffer[buffer_position++] = ',';
        UART1send(',');
        
        // Add temperature
        for (j = 0; temp_str[j] != '\0'; j++) {
            sd_buffer[buffer_position++] = temp_str[j];
            UART1send(temp_str[j]);
        }
        sd_buffer[buffer_position++] = '\n';
        UART1string("\r\n");
        
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
            // Turn off error LED if write succeeded
            P9OUT &= ~BIT7;
        } else {
            UART1string(">>> ERROR: SD write failed!\r\n\r\n");
            // Write failed - indicate error
            P9OUT |= BIT7;  // Turn on LED2
        }
    } else {
        UART1string(">>> SD NOT AVAILABLE: Data shown above (not written)\r\n\r\n");
        P9OUT |= BIT7;  // LED2 indicates no SD card
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
    while (!mmc_ping() && retry_count < 10) {
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
        P9OUT |= BIT7;  // Turn on LED2 to indicate SD error
    } else {
        UART1string("SUCCESS: SD card initialized!\r\n");
        UART1string("Card is ready for data logging\r\n");
        
        sd_initialized = 1;
        P9OUT &= ~BIT7; // Turn off LED2
        // Clear the buffer
        memset(sd_buffer, 0, SD_BUFFER_SIZE);
        UART1string("Ready to log data!\r\n");
        UART1string("========================================\r\n\r\n");
    }
}

void msp_init(void) {
    WDTCTL = WDTPW | WDTHOLD;     // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;         // Unlock ports from power manager
    
    // Initialize UART FIRST so we can debug
    UART1init(115200);
    __delay_cycles(200000);       // Let UART stabilize
    
    P9DIR |= BIT7;                //LED2 (P9.7) set as output
    P1DIR |= BIT0;                //LED1 (P1.0) set as output
    
    /*------ENERGY BAND 1-----*/
    P2DIR &= ~BIT1;               // Set pin P2.1 to be an input; energy band 1
    P2REN |=  BIT1;               // Enable internal pullup/pulldown resistor on P2.1
    P2OUT |=  BIT1;               // Pullup selected on P2.1
    
    P2IES |=  BIT1;               // Make P2.1 interrupt happen on the falling edge
    P2IFG &= ~BIT1;               // Clear the P2.1 interrupt flag
    P2IE  |=  BIT1;               // Enable P2.1 interrupt
    
    /*------ENERGY BAND 2-----*/
    P2DIR &= ~BIT2;               // Set pin P2.2 to be an input; energy band 2
    P2REN |=  BIT2;               // Enable internal pullup/pulldown resistor on P2.2
    P2OUT |=  BIT2;               // Pullup selected on P2.2
    
    P2IES |=  BIT2;               // Make P2.2 interrupt happen on the falling edge
    P2IFG &= ~BIT2;               // Clear the P2.2 interrupt flag
    P2IE  |=  BIT2;               // Enable P2.2 interrupt
    
    /*------ENERGY BAND 3-----*/
    P2DIR &= ~BIT3;               // Set pin P2.3 to be an input; energy band 3
    P2REN |=  BIT3;               // Enable internal pullup/pulldown resistor on P2.3
    P2OUT |=  BIT3;               // Pullup selected on P2.3
    
    P2IES |=  BIT3;               // Make P2.3 interrupt happen on the falling edge
    P2IFG &= ~BIT3;               // Clear the P2.3 interrupt flag
    P2IE  |=  BIT3;               // Enable P2.3 interrupt
    
    /*------ENERGY BAND 4-----*/
    P2DIR &= ~BIT4;               // Set pin P2.4 to be an input; energy band 4
    P2REN |=  BIT4;               // Enable internal pullup/pulldown resistor on P2.4
    P2OUT |=  BIT4;               // Pullup selected on P2.4
    
    P2IES |=  BIT4;               // Make P2.4 interrupt happen on the falling edge
    P2IFG &= ~BIT4;               // Clear the P2.4 interrupt flag
    P2IE  |=  BIT4;               // Enable P2.4 interrupt
    
    // RTC Initialization with proper sequencing
    RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
    RTCCTL1 = RTCBCD | RTCHOLD | RTCMODE;   // BCD mode, Calendar mode, Hold
    
    // Clear and set RTC values
    RTCYEAR = 0x2025;                       // Year 
    RTCMON = 0x10;                          // Month 
    RTCDAY = 0x09;                          // Day 
    RTCDOW = 0x04;                          // Day of week 
    RTCHOUR = 0x12;                         // Hour 
    RTCMIN = 0x00;                          // Minute 
    RTCSEC = 0x00;                          // Seconds 
    
    
    RTCCTL1 &= ~(RTCHOLD);                  // Start RTC
    RTCCTL0_H = 0;                          // Lock RTC
    
    // Initialize ADC for temperature sensing
    adc_init();

    UCA1IE |= UCRXIE;           // Enable USCI_A1 RX interrupt
    __enable_interrupt();       // Enable global interrupts
    
    P1OUT &= ~BIT0;             // Initialize LEDs to off
    P9OUT &= ~BIT7;
}

int main(void) {
    msp_init();
    
    // Small delay after init
    __delay_cycles(500000);
    
    // Send startup message
    UART1string("\r\n\r\n");
    UART1string("***************************************\r\n");
    UART1string("*   TIGR-T - Radiation Detector v2.3  *\r\n");
    UART1string("*    DEBUG MODE: SD Buffer Display    *\r\n");
    UART1string("***************************************\r\n");
    UART1string("System initializing...\r\n\r\n");
    
    // Display RTC status BEFORE SD init
    UART1string("=========== RTC Status Check ===========\r\n");
    char rtc_str[6];
    
    UART1string("Year  : 0x");
    hex_to_string_4(RTCYEAR, rtc_str);
    UART1string((unsigned char*)rtc_str);
    
    UART1string("\r\nMonth : 0x");
    bcd_to_string(RTCMON, rtc_str);
    UART1string((unsigned char*)rtc_str);

    UART1string("\r\nDay   : 0x");
    bcd_to_string(RTCDAY, rtc_str);
    UART1string((unsigned char*)rtc_str);
    
    UART1string("\r\nHour  : 0x");
    bcd_to_string(RTCHOUR, rtc_str);
    UART1string((unsigned char*)rtc_str);
    
    UART1string("\r\nMinute: 0x");
    bcd_to_string(RTCMIN, rtc_str);
    UART1string((unsigned char*)rtc_str);
    
    UART1string("\r\nSecond: 0x");
    bcd_to_string(RTCSEC, rtc_str);
    UART1string((unsigned char*)rtc_str);
    UART1string("\r\n========================================\r\n\r\n");
    
    // Test temperature sensor
    UART1string("======= Temperature Sensor Test ========\r\n");
    
    // Read and display raw ADC and calibration values
    while(!(REFCTL0 & REFGENRDY));  // Wait for reference
    ADC12CTL0 |= ADC12ENC | ADC12SC;
    while (ADC12CTL1 & ADC12BUSY);
    unsigned int raw_adc = ADC12MEM0;
    ADC12CTL0 &= ~ADC12ENC;
    
    unsigned int cal_30 = *((unsigned int *)0x1A1A);
    unsigned int cal_85 = *((unsigned int *)0x1A1C);
    
    UART1string("Raw ADC Value: ");
    char debug_val[12];
    uint_to_string(raw_adc, debug_val);
    UART1string((unsigned char*)debug_val);
    UART1string(" (should be ~2400-2600 at room temp)\r\n");
    
    UART1string("CAL_30C Value: ");
    uint_to_string(cal_30, debug_val);
    UART1string((unsigned char*)debug_val);
    UART1string("\r\n");
    
    UART1string("CAL_85C Value: ");
    uint_to_string(cal_85, debug_val);
    UART1string((unsigned char*)debug_val);
    UART1string("\r\n");
    
    int current_temp = read_temperature();
    UART1string("Calculated Temperature: ");
    char temp_str[12];
    int_to_string(current_temp, temp_str);
    UART1string((unsigned char*)temp_str);
    UART1string(" C\r\n");
    UART1string("========================================\r\n\r\n");
    
    sd_card_init();  // Initialize SD card
    
    // Optional: Write header to SD card
    UART1string("Writing CSV header to buffer...\r\n");
    strcpy((char*)sd_buffer, "Muon#,Band,Date,Time,TempC\n");
    buffer_position = strlen((char*)sd_buffer);
    UART1string("Header prepared: ");
    UART1string(sd_buffer);
    
    if (!sd_initialized) {
        UART1string("\r\nRunning in DEBUG mode (no SD card)\r\n");
        UART1string("All data will be displayed on terminal\r\n");
    }
    
    UART1string("\r\nSystem ready! Waiting for muon detections...\r\n");
    UART1string("Trigger P2.1, P2.2, P2.3, or P2.4 to simulate detection\r\n\r\n");
    
    while(1) {
        __low_power_mode_3();
        __delay_cycles(500000);   // Delay by half a second
        P1OUT &= ~BIT0;           // reset LEDs
        // Don't turn off LED2 if no SD card (keep it on as indicator)
        if (sd_initialized) {
            P9OUT &= ~BIT7;
        }
    }
}

#pragma vector=PORT2_VECTOR       // ISR for Port 2
__interrupt void ISRP1(void) {
    UART1string("\r\n>>> Interrupt detected! ");
    
    if(P2IFG & BIT4) {            // Energy band 4 caused the interrupt
        P1OUT |= BIT0;            // 1 1
        P9OUT |= BIT7;
        UART1string("Band 4 (Highest Energy)\r\n");
        save_reading(4);
    }
    else if(P2IFG & BIT3) {       // Energy band 3 caused the interrupt
        P1OUT |= BIT0;            // 1 0
        P9OUT &= ~BIT7;
        UART1string("Band 3\r\n");
        save_reading(3);
    }
    else if(P2IFG & BIT2) {       // Energy band 2 caused the interrupt
        P1OUT &= ~BIT0;           // 0 1
        P9OUT |= BIT7;
        UART1string("Band 2\r\n");
        save_reading(2);
    }
    else if(P2IFG & BIT1) {       // Energy band 1 caused the interrupt
        P1OUT &= ~BIT0;           // 0 0
        P9OUT &= ~BIT7;
        UART1string("Band 1 (Lowest Energy)\r\n");
        save_reading(1);
    }
    muon_count++;
    
    P2IFG &= ~(BIT1 | BIT2 | BIT3 | BIT4);  // clear interrupt flags
    __low_power_mode_off_on_exit();
}