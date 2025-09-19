// tigr_mmc.c
// MMC/SD Card Interface Implementation for TIGR Project
// MSP430FR6989 using eUSCI_B0 SPI

#include "tigr_mmc.h"
#include "tigr_config.h"

// SPI Initialize for MSP430FR6989
void spi_init(void) {
    // Configure SPI pins
    P1SEL0 |= BIT4 | BIT5 | BIT6;  // P1.4=UCB0SIMO, P1.5=UCB0SOMI, P1.6=UCB0CLK
    P1SEL1 &= ~(BIT4 | BIT5 | BIT6);
    
    // Configure CS pin as output
    SD_CS_DIR |= SD_CS_PIN;
    CS_HIGH();
    
    // Configure Card Detect pin as input with pullup
    SD_CD_DIR &= ~SD_CD_PIN;
    P1REN |= SD_CD_PIN;
    P1OUT |= SD_CD_PIN;
    
    // Configure eUSCI_B0 for SPI Master mode
    UCB0CTLW0 |= UCSWRST;                      // Put state machine in reset
    UCB0CTLW0 |= UCMST | UCSYNC | UCCKPL | UCMSB; // 3-pin, 8-bit SPI master
    UCB0CTLW0 |= UCSSEL__SMCLK;                // SMCLK as clock source
    UCB0BR0 = 0x02;                            // fBitClock = fSMCLK/2
    UCB0BR1 = 0;
    UCB0CTLW0 &= ~UCSWRST;                     // Initialize USCI state machine
}

// Send byte via SPI
unsigned char spi_send_byte(unsigned char data) {
    while (!(UCB0IFG & UCTXIFG));              // Wait for TX buffer ready
    UCB0TXBUF = data;                          // Send byte
    while (!(UCB0IFG & UCRXIFG));              // Wait for RX complete
    return UCB0RXBUF;                          // Return received byte
}

// Send frame via SPI
void spi_send_frame(unsigned char* buffer, unsigned int length) {
    unsigned int i;
    for (i = 0; i < length; i++) {
        buffer[i] = spi_send_byte(buffer[i]);
    }
}

// Read frame via SPI
void spi_read_frame(unsigned char* buffer, unsigned int length) {
    unsigned int i;
    for (i = 0; i < length; i++) {
        buffer[i] = spi_send_byte(0xFF);
    }
}

// Initialize MMC/SD card
unsigned char mmc_init(void) {
    int i;
    unsigned char response;
    
    // Initialize SPI
    spi_init();
    
    // Send 80 clock pulses with CS high
    CS_HIGH();
    for(i = 0; i < 10; i++) {
        spi_send_byte(0xFF);
    }
    
    // Put card in idle state
    if (mmc_go_idle() != 0) {
        return MMC_INIT_ERROR;
    }
    
    // Send CMD1 until card is ready
    response = 0x01;
    i = 0;
    while (response == 0x01 && i < MMC_INIT_TIMEOUT) {
        CS_HIGH();
        spi_send_byte(0xFF);
        CS_LOW();
        mmc_send_cmd(MMC_SEND_OP_COND, 0x00, 0xFF);
        response = mmc_get_response();
        i++;
    }
    
    CS_HIGH();
    spi_send_byte(0xFF);
    
    if (i >= MMC_INIT_TIMEOUT) {
        return MMC_TIMEOUT_ERROR;
    }
    
    // Set block length to 512 bytes
    if (mmc_set_block_length(MMC_BLOCK_SIZE) != MMC_SUCCESS) {
        return MMC_BLOCK_SET_ERROR;
    }
    
    return MMC_SUCCESS;
}

// Put MMC in idle state
unsigned char mmc_go_idle(void) {
    CS_LOW();
    mmc_send_cmd(MMC_GO_IDLE_STATE, 0, 0x95);
    
    if (mmc_get_response() != MMC_R1_IDLE_STATE) {
        CS_HIGH();
        return MMC_RESPONSE_ERROR;
    }
    
    CS_HIGH();
    return MMC_SUCCESS;
}

// Send command to MMC
void mmc_send_cmd(unsigned char cmd, unsigned long arg, unsigned char crc) {
    spi_send_byte(cmd | 0x40);
    spi_send_byte(arg >> 24);
    spi_send_byte(arg >> 16);
    spi_send_byte(arg >> 8);
    spi_send_byte(arg);
    spi_send_byte(crc);
}

// Get response from MMC
unsigned char mmc_get_response(void) {
    int i = 0;
    unsigned char response;
    
    while (i <= MMC_RESPONSE_TIMEOUT) {
        response = spi_send_byte(0xFF);
        if (response == 0x00 || response == 0x01) {
            break;
        }
        i++;
    }
    return response;
}

// Get specific response from MMC
unsigned char mmc_get_xx_response(unsigned char resp) {
    int i = 0;
    unsigned char response;
    
    while (i <= 1000) {
        response = spi_send_byte(0xFF);
        if (response == resp) {
            break;
        }
        i++;
    }
    return response;
}

// Check if MMC is busy
unsigned char mmc_check_busy(void) {
    int i = 0;
    unsigned char response;
    
    do {
        response = spi_send_byte(0xFF);
        i++;
    } while (response == 0x00 && i < 1000);
    
    return (i < 1000) ? MMC_SUCCESS : MMC_TIMEOUT_ERROR;
}

// Set block length
unsigned char mmc_set_block_length(unsigned long length) {
    CS_LOW();
    mmc_send_cmd(MMC_SET_BLOCKLEN, length, 0xFF);
    
    if (mmc_get_response() != MMC_R1_RESPONSE) {
        CS_HIGH();
        return MMC_BLOCK_SET_ERROR;
    }
    
    CS_HIGH();
    spi_send_byte(0xFF);
    return MMC_SUCCESS;
}

// Read block from MMC
unsigned char mmc_read_block(unsigned long address, unsigned char *buffer) {
    int i;
    unsigned char response;
    
    CS_LOW();
    
    // Send read command
    mmc_send_cmd(MMC_READ_SINGLE_BLOCK, address, 0xFF);
    
    // Check response
    response = mmc_get_response();
    if (response != MMC_R1_RESPONSE) {
        CS_HIGH();
        return MMC_RESPONSE_ERROR;
    }
    
    // Wait for data token
    if (mmc_get_xx_response(MMC_START_DATA_BLOCK_TOKEN) != MMC_START_DATA_BLOCK_TOKEN) {
        CS_HIGH();
        return MMC_DATA_TOKEN_ERROR;
    }
    
    // Read data
    for (i = 0; i < MMC_BLOCK_SIZE; i++) {
        buffer[i] = spi_send_byte(0xFF);
    }
    
    // Read and discard CRC
    spi_send_byte(0xFF);
    spi_send_byte(0xFF);
    
    CS_HIGH();
    spi_send_byte(0xFF);
    
    return MMC_SUCCESS;
}

// Write block to MMC
unsigned char mmc_write_block(unsigned long address, unsigned char *buffer) {
    int i;
    unsigned char response;
    
    CS_LOW();
    
    // Send write command
    mmc_send_cmd(MMC_WRITE_BLOCK, address, 0xFF);
    
    // Check response
    if (mmc_get_response() != MMC_R1_RESPONSE) {
        CS_HIGH();
        return MMC_RESPONSE_ERROR;
    }
    
    // Send dummy byte
    spi_send_byte(0xFF);
    
    // Send data token
    spi_send_byte(MMC_START_DATA_BLOCK_TOKEN);
    
    // Send data
    for (i = 0; i < MMC_BLOCK_SIZE; i++) {
        spi_send_byte(buffer[i]);
    }
    
    // Send dummy CRC
    spi_send_byte(0xFF);
    spi_send_byte(0xFF);
    
    // Check data response
    response = spi_send_byte(0xFF);
    if ((response & 0x1F) != 0x05) {
        CS_HIGH();
        return MMC_WRITE_ERROR;
    }
    
    // Wait for card to complete write
    if (mmc_check_busy() != MMC_SUCCESS) {
        CS_HIGH();
        return MMC_TIMEOUT_ERROR;
    }
    
    CS_HIGH();
    spi_send_byte(0xFF);
    
    return MMC_SUCCESS;
}

// Read MMC register
unsigned char mmc_read_register(unsigned char cmd_register, unsigned char length, unsigned char *buffer) {
    unsigned char i;
    
    CS_LOW();
    
    // Send command
    mmc_send_cmd(cmd_register, 0x00000000, 0xFF);
    
    // Check response
    if (mmc_get_response() != MMC_R1_RESPONSE) {
        CS_HIGH();
        return MMC_RESPONSE_ERROR;
    }
    
    // Wait for data token
    if (mmc_get_xx_response(MMC_START_DATA_BLOCK_TOKEN) != MMC_START_DATA_BLOCK_TOKEN) {
        CS_HIGH();
        return MMC_DATA_TOKEN_ERROR;
    }
    
    // Read data
    for (i = 0; i < length; i++) {
        buffer[i] = spi_send_byte(0xFF);
    }
    
    // Read and discard CRC
    spi_send_byte(0xFF);
    spi_send_byte(0xFF);
    
    CS_HIGH();
    spi_send_byte(0xFF);
    
    return MMC_SUCCESS;
}

// Read card size from CSD register
unsigned long mmc_read_card_size(void) {
    unsigned char csd[16];
    unsigned long capacity = 0;
    unsigned int c_size, c_size_mult, read_bl_len;
    unsigned int mult, block_len;
    
    if (mmc_read_register(MMC_READ_CSD, 16, csd) != MMC_SUCCESS) {
        return 0;
    }
    
    // Extract fields from CSD
    read_bl_len = csd[5] & 0x0F;
    
    c_size = ((unsigned int)(csd[6] & 0x03) << 10) |
             ((unsigned int)csd[7] << 2) |
             ((unsigned int)(csd[8] & 0xC0) >> 6);
    
    c_size_mult = ((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7);
    
    // Calculate capacity
    mult = 1 << (c_size_mult + 2);
    block_len = 1 << read_bl_len;
    capacity = (unsigned long)(c_size + 1) * mult * block_len;
    
    return capacity;
}

// Check if card is present
unsigned char mmc_ping(void) {
    if (CARD_PRESENT()) {
        return MMC_SUCCESS;
    }
    return MMC_INIT_ERROR;
}
