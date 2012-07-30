#include "minunit.h"
#include <lcthw/ringbuffer.h>

char *test_ringbuffer()
{
  RingBuffer* r = RingBuffer_create(100);

  printf("Available space: %d\n", RingBuffer_available_space(r));

  printf("Write: %d\n", RingBuffer_write(r, (unsigned char *)"abcde\0", 6));
  printf("Available data: %d\n", RingBuffer_available_data(r));

  unsigned char buffer[10];
  int count = RingBuffer_read(r, buffer, 4);  

  printf("Read %d %s\n", count, buffer);

  RingBuffer_destroy(r);
  return NULL;
}


char *all_tests() {
    mu_suite_start();

    mu_run_test(test_ringbuffer);

    return NULL;
}

RUN_TESTS(all_tests);