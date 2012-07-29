#include "minunit.h"
#include <lcthw/ringbuffer.h>

char *test_ringbuffer()
{
  RingBuffer* r = RingBuffer_create(100);

  printf("Available data: %d\n", RingBuffer_available_space(r));

  RingBuffer_write(r, "abcde\0", 6);

  char buffer[10];
  int count = RingBuffer_read(r, buffer, 10);  

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