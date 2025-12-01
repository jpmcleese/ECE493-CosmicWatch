// UART.c
// UART implementation for TIGR project
// Adapted for MSP430FR2355 - uses eUSCI_A0 on P1.6/P1.7
//
// MSP430FR2355 UART Pinout (Backchannel UART on LaunchPad):
//   P1.6 = UCA0RXD (Receive)
//   P1.7 = UCA0TXD (Transmit)
//
// Note: The FR2355 LaunchPad uses UCA0 for backchannel UART, not UCA1

#include <msp430.h>
#include "UART.h"

// Software trim function for DCO
void Software_Trim(void);

#define MCLK_FREQ_MHZ 8                     // MCLK = 8MHz

// Initialize clock to 8MHz for UART operation
static void initClockTo8MHz(void) {
    // Disable FLL
    __bis_SR_register(SCG0);
    
    // Set REFO as FLL reference source
    CSCTL3 |= SELREF__REFOCLK;
    
    // Configure DCO for 8MHz: DCOFTRIM=3, DCO Range = 8MHz
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_3;
    CSCTL2 = FLLD_0 + 243;                  // DCODIV = 8MHz
    
    __delay_cycles(3);
    
    // Enable FLL
    __bic_SR_register(SCG0);
    
    // Software Trim to get the best DCOFTRIM value
    Software_Trim();
    
    // Set default REFO(~32768Hz) as ACLK source
    // DCODIV as MCLK and SMCLK source
    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;
}

// Software Trim to get the best DCOFTRIM value
void Software_Trim(void) {
    unsigned int oldDcoTap = 0xffff;
    unsigned int newDcoTap = 0xffff;
    unsigned int newDcoDelta = 0xffff;
    unsigned int bestDcoDelta = 0xffff;
    unsigned int csCtl0Copy = 0;
    unsigned int csCtl1Copy = 0;
    unsigned int csCtl0Read = 0;
    unsigned int csCtl1Read = 0;
    unsigned int dcoFreqTrim = 3;
    unsigned char endLoop = 0;

    do {
        CSCTL0 = 0x100;                         // DCO Tap = 256
        do {
            CSCTL7 &= ~DCOFFG;                  // Clear DCO fault flag
        } while (CSCTL7 & DCOFFG);              // Test DCO fault flag

        // Wait FLL lock status (FLLUNLOCK) to be stable
        __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ);
        
        while((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) && ((CSCTL7 & DCOFFG) == 0));

        csCtl0Read = CSCTL0;                   // Read CSCTL0
        csCtl1Read = CSCTL1;                   // Read CSCTL1

        oldDcoTap = newDcoTap;                 // Record DCOTAP value of last time
        newDcoTap = csCtl0Read & 0x01ff;       // Get DCOTAP value of this time
        dcoFreqTrim = (csCtl1Read & 0x0070) >> 4; // Get DCOFTRIM value

        if(newDcoTap < 256) {                  // DCOTAP < 256
            newDcoDelta = 256 - newDcoTap;     // Delta value between DCPTAP and 256
            if((oldDcoTap != 0xffff) && (oldDcoTap >= 256)) // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else {
                dcoFreqTrim--;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim << 4);
            }
        }
        else {                                 // DCOTAP >= 256
            newDcoDelta = newDcoTap - 256;     // Delta value between DCPTAP and 256
            if(oldDcoTap < 256)                // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else {
                dcoFreqTrim++;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim << 4);
            }
        }

        if(newDcoDelta < bestDcoDelta) {       // Record DCOTAP closest to 256
            csCtl0Copy = csCtl0Read;
            csCtl1Copy = csCtl1Read;
            bestDcoDelta = newDcoDelta;
        }

    } while(endLoop == 0);                     // Poll until endLoop == 1

    CSCTL0 = csCtl0Copy;                       // Reload locked DCOTAP
    CSCTL1 = csCtl1Copy;                       // Reload locked DCOFTRIM
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked
}

// Initialize UART1 (actually UCA0 on FR2355)
// For compatibility, we keep the function name UART1init
// P1.6 = UCA0RXD, P1.7 = UCA0TXD
void UART1init(unsigned long BaudRate) {
    // Configure UART pins
    P1SEL0 |= BIT6 | BIT7;                    // Set P1.6 and P1.7 as UART function
    
    // Initialize clock to 8MHz
    initClockTo8MHz();
    
    // Configure UART
    UCA0CTLW0 |= UCSWRST;                     // Put eUSCI in reset
    UCA0CTLW0 |= UCSSEL__SMCLK;               // Use SMCLK as clock source
    
    // Baud Rate calculation for 8MHz SMCLK
    switch(BaudRate) {
        case 9600:
            // 8000000/(16*9600) = 52.083
            // UCBRSx = 0x49, UCBRFx = 1
            UCA0BR0 = 52;
            UCA0BR1 = 0x00;
            UCA0MCTLW = 0x4900 | UCOS16 | UCBRF_1;
            break;
            
        case 57600:
            // 8000000/(16*57600) = 8.68
            // UCBRSx = 0xF7, UCBRFx = 10
            UCA0BR0 = 8;
            UCA0BR1 = 0x00;
            UCA0MCTLW = 0xF700 | UCOS16 | UCBRF_10;
            break;
            
        case 115200:
        default:
            // 8000000/(16*115200) = 4.34
            // UCBRSx = 0x55, UCBRFx = 5
            UCA0BR0 = 4;
            UCA0BR1 = 0x00;
            UCA0MCTLW = 0x5500 | UCOS16 | UCBRF_5;
            break;
    }
    
    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
}

// Send a single character via UART
void UART1send(unsigned char data) {
    // Wait for any ongoing transmission to complete
    while(!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = data;
}

// Send a null-terminated string via UART
void UART1string(unsigned char str[]) {
    unsigned char *nextChr = str;
    unsigned char c;
    while((c = *nextChr++) != 0) {
        UART1send(c);
    }
}

// Receive a character via UART (non-blocking)
unsigned char UART1receive(void) {
    // Return immediately if no data available
    if(!(UCA0IFG & UCRXIFG)) return 0;
    return UCA0RXBUF;
}
