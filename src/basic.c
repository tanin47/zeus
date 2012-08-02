#include <basic.h>
#include <string.h>

unsigned int chars_to_int(unsigned char *arr, int len) {
  unsigned int ret = 0;

  int i;
  for (i=0;i<len;i++) {
    ret = ret << 8;
    ret |= arr[i];
  }

  return ret;
}


unsigned int chars_to_int_little_endian(unsigned char *arr, int len) {
  unsigned int ret = 0;

  int i;
  for (i=len-1;i>=0;i--) {
    ret = ret << 8;
    ret |= arr[i];
  }

  return ret;
}


void int_to_byte_array(int number, unsigned char *arr, int start, int length) {
  int i;
  for (i=length + start - 1;i>=start;i--) {
    arr[i] = (unsigned char)(number & 0xFF);
    number = number >> 8;
  }
}

double chars_to_double(unsigned char *arr, int len) {
  double a;
  memcpy(&a, arr, len);
  return a;
}
