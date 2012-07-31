#include "minunit.h"
#include <amf0.h>
#include <dbg.h>

int test_serialize_invoke_message() {
  Amf0InvokeMessage *msg = amf0_create_invoke_message();

  msg->command = bfromcstr("Hello");
  msg->transaction_id = 1.0;

  Amf0ObjectValue *val_0 = malloc(sizeof(Amf0ObjectValue));
  val_0->type = AMF0_STRING;
  val_0->string = bfromcstr("Value");
  Hashmap_set(msg->arguments, bfromcstr("StringKey"), val_0);

  Amf0ObjectValue *val_1 = malloc(sizeof(Amf0ObjectValue));
  val_1->type = AMF0_NUMBER;
  val_1->number = 1.0;
  Hashmap_set(msg->arguments, bfromcstr("NumberKey"), val_1);

  unsigned char buffer[65536];
  int count = amf0_serialize_invoke_message(buffer, msg);
  
  int i;
  for (i=0;i<60;i++) {
    printf("%02x ", buffer[i]);
  }
  printf("\n");

  check(count == 60, "The length of bytes is not correct, expect %d, got %d", 34, count);

  amf0_destroy_invoke_message(msg);
  msg = NULL;

  return 1;
error:
  return 0;
}

int test_string()
{
  unsigned char output[10];
  amf0_serialize_string(output, bfromcstr("abcdefg"));

  unsigned char correct[10] = {
    0x02, 0x00, 0x07,
    'a', 'b', 'c', 'd', 'e', 'f', 'g'
  };

  int i;
  for (i=0;i<10;i++) {
    check(output[i] == correct[i], "The byte %d is not correct (expect %x, got %x).", i, correct[i], output[i]); 
  }
  
  // unsigned char string[7];
  // amf0_deserialize_string(string, output, 0, 10);

  // for (i=0;i<7;i++) {
  //   check(string[i] == correct[i + 3], "The char %d is not correct (expect %c, got %c).", i, string[i], correct[i]); 
  // } 

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
  mu_assert(test_serialize_invoke_message(), "test_string() failed.");

  return NULL;
}


char *all_tests() {
    mu_suite_start();

    mu_run_test(test_functions);

    return NULL;
}

RUN_TESTS(all_tests);