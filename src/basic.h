#ifndef __basic_h__
#define __basic_h__

#define countof(a) ((sizeof(a)/sizeof((a)[0])))

unsigned int chars_to_int(unsigned char *arr, int len);
unsigned int chars_to_int_little_endian(unsigned char *arr, int len);

void int_to_byte_array(int number, unsigned char *arr, int start, int length);

double chars_to_double(unsigned char *arr, int len);

#endif