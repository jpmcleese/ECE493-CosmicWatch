// tigr_mmc.h
// MMC/SD Card Interface Header for TIGR Project
// MSP430FR6989 Implementation

#ifndef _TIGR_MMC_H
#define _TIGR_MMC_H

#include <msp430.h>

//-----------------------------------------------------------------------------
// Pin Definitions for MSP430FR6989
//-----------------------------------------------------------------------------
// Using eUSCI_B0 for SPI:
// P1.4 = UCB0SIMO (MOSI)
// P1.5 = UCB0SOMI (MISO)  
// P1.6 = UCB0CLK (SCLK)
// P1.3 = CS (Chip Select) - GPIO
// P1.2 = Card Detect - GPIO (optional)

#define SD_CS_OUT       P1OUT
#define SD_CS_DIR       P1DIR
#define SD_CS_PIN       BIT3

#define SD_CD_IN        P1IN
#define SD_CD_DIR       P1DIR
#define SD_CD_PIN       BIT2

// Chip Select Macros
#define CS_HIGH()       SD_CS_OUT |= SD_CS_PIN
#define CS_LOW()        SD_CS_OUT &= ~SD_CS_PIN
#define CARD_PRESENT()  (!(SD_CD_IN & SD_CD_PIN))

//-----------------------------------------------------------------------------
// MMC/SD Commands (SPI Mode)
//-----------------------------------------------------------------------------
#define MMC_GO_IDLE_STATE          0x40     // CMD0  - Software reset
#define MMC_SEND_OP_COND           0x41     // CMD1  - Initialize card
#define MMC_SEND_IF_COND           0x48     // CMD8  - Check voltage range (SD v2)
#define MMC_READ_CSD               0x49     // CMD9  - Read Card Specific Data
#define MMC_SEND_CID               0x4A     // CMD10 - Read Card ID
#define MMC_STOP_TRANSMISSION      0x4C     // CMD12 - Stop multiple block read
#define MMC_SEND_STATUS            0x4D     // CMD13 - Request card status
#define MMC_SET_BLOCKLEN           0x50     // CMD16 - Set block length
#define MMC_READ_SINGLE_BLOCK      0x51     // CMD17 - Read single block
#define MMC_READ_MULTIPLE_BLOCK    0x52     // CMD18 - Read multiple blocks
#define MMC_WRITE_BLOCK            0x58     // CMD24 - Write single block
#define MMC_WRITE_MULTIPLE_BLOCK   0x59     // CMD25 - Write multiple blocks
#define MMC_APP_CMD                0x77     // CMD55 - Next command is app-specific
#define MMC_READ_OCR               0x7A     // CMD58 - Read OCR register
#define SD_SEND_OP_COND            0x69     // ACMD41 - Initialize SD card

//-----------------------------------------------------------------------------
// MMC/SD Tokens
//-----------------------------------------------------------------------------
#define MMC_START_DATA_BLOCK_TOKEN          0xFE   // Start single block read/write
#define MMC_START_DATA_MULTIPLE_BLOCK_WRITE 0xFC   // Start multiple block write
#define MMC_STOP_DATA_MULTIPLE_BLOCK_WRITE  0xFD   // Stop multiple block write

//-----------------------------------------------------------------------------
// MMC/SD Responses
//-----------------------------------------------------------------------------
#define MMC_R1_RESPONSE       0x00   // No error
#define MMC_R1_IDLE_STATE     0x01   // In idle state
#define MMC_R1_ERASE_RESET    0x02   // Erase reset
#define MMC_R1_ILLEGAL_CMD    0x04   // Illegal command
#define MMC_R1_CRC_ERROR      0x08   // CRC error
#define MMC_R1_ERASE_ERROR    0x10   // Erase sequence error
#define MMC_R1_ADDRESS_ERROR  0x20   // Address error
#define MMC_R1_PARAM_ERROR    0x40   // Parameter error

//-----------------------------------------------------------------------------
// Error Codes
//-----------------------------------------------------------------------------
#define MMC_SUCCESS           0x00
#define MMC_BLOCK_SET_ERROR   0x01
#define MMC_RESPONSE_ERROR    0x02
#define MMC_DATA_TOKEN_ERROR  0x03
#define MMC_INIT_ERROR        0x04
#define MMC_CRC_ERROR         0x10
#define MMC_WRITE_ERROR       0x11
#define MMC_OTHER_ERROR       0x12
#define MMC_TIMEOUT_ERROR     0xFF

//-----------------------------------------------------------------------------
// Configuration Constants
//-----------------------------------------------------------------------------
#define MMC_BLOCK_SIZE        512    // Standard SD card block size
#define MMC_INIT_TIMEOUT      1000   // Initialization timeout loops
#define MMC_RESPONSE_TIMEOUT  64     // Response timeout loops

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

// SPI Functions
void spi_init(void);
unsigned char spi_send_byte(unsigned char data);
void spi_send_frame(unsigned char* buffer, unsigned int length);
void spi_read_frame(unsigned char* buffer, unsigned int length);

// MMC/SD Card Functions
unsigned char mmc_init(void);
unsigned char mmc_go_idle(void);
void mmc_send_cmd(unsigned char cmd, unsigned long arg, unsigned char crc);
unsigned char mmc_get_response(void);
unsigned char mmc_get_xx_response(unsigned char response);
unsigned char mmc_check_busy(void);

// Block Operations
unsigned char mmc_set_block_length(unsigned long length);
unsigned char mmc_read_block(unsigned long address, unsigned char *buffer);
unsigned char mmc_write_block(unsigned long address, unsigned char *buffer);

// Utility Functions
unsigned char mmc_read_register(unsigned char cmd_register, unsigned char length, unsigned char *buffer);
unsigned long mmc_read_card_size(void);
unsigned char mmc_ping(void);  // Check if card is present

// Sector-based Operations (512 bytes per sector)
#define mmc_read_sector(sector, buffer)  mmc_read_block((sector)*512UL, buffer)
#define mmc_write_sector(sector, buffer) mmc_write_block((sector)*512UL, buffer)

//-----------------------------------------------------------------------------
// Data Structure for Card Information
//-----------------------------------------------------------------------------
typedef struct {
    unsigned long capacity;      // Card capacity in bytes
    unsigned int  block_size;    // Block size in bytes
    unsigned char card_type;     // 0=MMC, 1=SD v1, 2=SD v2, 3=SDHC
    unsigned char initialized;   // 0=not initialized, 1=initialized
} MMC_CardInfo;

#endif /* _TIGR_MMC_H */