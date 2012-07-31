#include <basic.h>

unsigned int chars_to_int(unsigned char *arr, int len) {
  unsigned int ret = 0;
  unsigned int base = 1;

  int i;
  for (i=len-1;i>=0;i--) {
    ret += arr[i] * base;
    base = base * 256;
  }

  return ret;
}


unsigned int chars_to_int_little_endian(unsigned char *arr, int len) {
  unsigned int ret = 0;
  unsigned int base = 1;

  int i;
  for (i=0;i<len;i++) {
    ret += arr[i] * base;
    base = base * 256;
  }

  return ret;
}


void int_to_byte_array(int number, unsigned char *arr, int start, int end) {
  int i;
  for (i=end + start - 1;i>=start;i--) {
    arr[i] = number & 0xFF;
    number = number >> 8;
  }
}

double chars_to_double(unsigned char *arr, int len) {
  double a;
  memcpy(&a, arr, len);
  return a;
}
