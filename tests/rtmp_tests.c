#include "minunit.h"
#include <rtmp.h>


char *test_enum()
{
  Rtmp* r = malloc(sizeof(Rtmp));

  r->state = WAIT_FOR_C0;
  printf("State: %d\n", r->state);

  r->state = WAIT_FOR_C1;
  printf("State: %d\n", r->state);

  r->state = WAIT_FOR_C2;
  printf("State: %d\n", r->state);

  free(r);
}


char *all_tests() {
    mu_suite_start();

    mu_run_test(test_enum);

    return NULL;
}

RUN_TESTS(all_tests);