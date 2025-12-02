// tigr_utils.h
// Utility functions for TIGR project
// String conversion and helper functions

#ifndef _TIGR_UTILS_H
#define _TIGR_UTILS_H

// String conversion functions
void uint_to_string(unsigned int num, char* str);
void int_to_string(int num, char* str);
void bcd_to_string(unsigned char bcd, char* str);
void hex_to_string_4(unsigned int hex, char* str);

#endif /* _TIGR_UTILS_H */
