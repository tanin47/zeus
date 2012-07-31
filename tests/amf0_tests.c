#include "minunit.h"
#include <amf0.h>
#include <dbg.h>

int test_string()
{
  unsigned char output[10];
  amf0_serialize_string(output, "abcdefg", 7);

  unsigned char correct[10] = {
    0x02, 0x00, 0x07,
    'a', 'b', 'c', 'd', 'e', 'f', 'g'
  };

  int i;
  for (i=0;i<10;i++) {
    check(output[i] == correct[i], "The byte %d is not correct (expect %x, got %x).", i, correct[i], output[i]); 
  }
  
  unsigned char string[7];
  amf0_deserialize_string(string, output, 0, 10);

  for (i=0;i<7;i++) {
    check(string[i] == correct[i + 3], "The char %d is not correct (expect %c, got %c).", i, string[i], correct[i]); 
  } 

  return 1;
error:
  return 0;
}

char *test_number()
{

}

char *test_functions()
{
  mu_assert(test_string(), "test_string() failed.");

  return NULL;
}


char *all_tests() {
    mu_suite_start();

    mu_run_test(test_functions);

    return NULL;
}

RUN_TESTS(all_tests);