// tigr_utils.c
// Utility functions implementation for TIGR project
// Mainly for UART communication

#include "tigr_utils.h"

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
