#include "minunit.h"
#include <amf0.h>
#include <dbg.h>

int test_serialize_and_deserialize_invoke_message() {
  Amf0InvokeMessage *msg = amf0_create_invoke_message();

  msg->command = bfromcstr("Hello");
  msg->transaction_id = 1.0;

  Amf0ObjectValue *val, *sub_val;

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_NUMBER;
  val->number = 1.0;
  Hashmap_set(msg->arguments, bfromcstr("NumberKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_BOOLEAN;
  val->boolean = 1;
  Hashmap_set(msg->arguments, bfromcstr("BooleanKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_STRING;
  val->string = bfromcstr("Value");
  Hashmap_set(msg->arguments, bfromcstr("StringKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_OBJECT;
  val->object = Hashmap_create(NULL, NULL);

    sub_val = malloc(sizeof(Amf0ObjectValue));
    sub_val->type = AMF0_STRING;
    sub_val->string = bfromcstr("SomeValueInObject");

    Hashmap_set(val->object, bfromcstr("NothingInObject"), sub_val);
  Hashmap_set(msg->arguments, bfromcstr("ObjectKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_NULL;
  Hashmap_set(msg->arguments, bfromcstr("NullKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_UNDEFINED;
  Hashmap_set(msg->arguments, bfromcstr("UndefinedKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_REFERENCE;
  val->reference = 0x1234;
  Hashmap_set(msg->arguments, bfromcstr("ReferencedKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_ECMA_ARRAY;
  val->ecma_array = Hashmap_create(NULL, NULL);

    sub_val = malloc(sizeof(Amf0ObjectValue));
    sub_val->type = AMF0_STRING;
    sub_val->string = bfromcstr("SomeValue");

    Hashmap_set(val->ecma_array, bfromcstr("Nothing"), sub_val);
  Hashmap_set(msg->arguments, bfromcstr("EcmaArrayKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_STRICT_ARRAY;
  val->strict_array = malloc(sizeof(Amf0StrictArray));

    val->strict_array->length = 2;
    val->strict_array->data = calloc(2, sizeof(Amf0ObjectValue *));

    val->strict_array->data[0] = malloc(sizeof(Amf0ObjectValue));
    val->strict_array->data[0]->type = AMF0_STRING;
    val->strict_array->data[0]->string = bfromcstr("FirstElement");

    val->strict_array->data[1] = malloc(sizeof(Amf0ObjectValue));
    val->strict_array->data[1]->type = AMF0_NUMBER;
    val->strict_array->data[1]->number = 1.0;
  
  Hashmap_set(msg->arguments, bfromcstr("StrictArrayKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_DATE;
  val->date = 1234.5678;
  Hashmap_set(msg->arguments, bfromcstr("DateKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_LONG_STRING;
  val->string = bfromcstr("LongValue");
  Hashmap_set(msg->arguments, bfromcstr("LongStringKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_UNSUPPORTED;
  Hashmap_set(msg->arguments, bfromcstr("UnsupportedKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_XML_DOCUMENT;
  val->xml_document = bfromcstr("<xml>");
  Hashmap_set(msg->arguments, bfromcstr("XmlKey"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_TYPED_OBJECT;
  val->typed_object = malloc(sizeof(Amf0TypedObject));
  val->typed_object->class_name = bfromcstr("SomeClassName");
  val->typed_object->object = Hashmap_create(NULL, NULL);

    sub_val = malloc(sizeof(Amf0ObjectValue));
    sub_val->type = AMF0_STRING;
    sub_val->string = bfromcstr("SomeValueInTypedObject");

    Hashmap_set(val->typed_object->object, bfromcstr("NothingInTypedObject"), sub_val);
  Hashmap_set(msg->arguments, bfromcstr("TypedKey"), val);

  unsigned char buffer[65536];
  int count = amf0_serialize_invoke_message(buffer, msg);
  
  printf("length=%d\n", count);
  int i;
  for (i=0;i<count;i++) {
    printf("%02x ", buffer[i]);
  }
  printf("\n\n\n");

  amf0_destroy_invoke_message(msg);

  msg = amf0_create_invoke_message();
  amf0_deserialize_invoke_message(msg, buffer);

  printf("Deserialization\n");
  printf("Command: %s\n", bdata(msg->command));
  printf("Transaction ID: %f\n", msg->transaction_id);
  printf("Arguments:\n");
  print_amf0_object(msg->arguments);
  printf("\n");

  amf0_destroy_invoke_message(msg);

  return 1;
error:
  return 0;
}

int test_deserialize_real_invoke_message()
{
  unsigned char input[] = {
    0x02, 0x00, 0x07, 0x63, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, //= String "connect"
    0x00, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //= Number 1.0
    0x03, // Object 
    0x00, 0x03, 0x61, 0x70, 0x70, //app
    0x02, 0x00, 0x00, //<Empty string>

    0x00, 0x08, 0x66, 0x6C, 0x61, 0x73, 0x68, 0x56, 0x65, 0x72, //flashVer
    0x02, 0x00, 0x10, 0x4C, 0x4E, 0x58, 0x20, 0x31, 0x31, 0x2C, 0x32, 0x2C, 0x32, 0x30, 0x32, 0x2C, 0x32, 0x32, 0x38, //Something

    0x00, 0x06, 0x73, 0x77, 0x66, 0x55, 0x72, 0x6C, //some key
    0x06, // Undefined 

    0x00, 0x05, 0x74, 0x63, 0x55, 0x72, 0x6C, 
    0x02, 0x00, 0x15, 0x72, 0x74, 0x6D, 0x70, 0x3A, 0x2F, 0x2F, 0x6C, 0x6F, 0x63, 0x61, 0x6C, 0x68, 0x6F, 0x73, 0x74, 0x3A, 0x31, 0x39, 0x33, 0x35, 

    0x00, 0x04, 0x66, 0x70, 0x61, 0x64, 
    0x01, 0x00,

    0x00, 0x0C, 0x63, 0x61, 0x70, 0x61, 0x62, 0x69, 0x6C, 0x69, 0x74, 0x69, 0x65, 0x73,
    0x00, 0x40, 0x6D, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x0B, 0x61, 0x75, 0x64, 0x69, 0x6F, 0x43, 0x6F, 0x64, 0x65, 0x63, 0x73,
    0x00, 0x40, 0xAB, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x0B, 0x76, 0x69, 0x64, 0x65, 0x6F, 0x43, 0x6F, 0x64, 0x65, 0x63, 0x73,
    0x00, 0x40, 0x6F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x0D, 0x76, 0x69, 0x64, 0x65, 0x6F, 0x46, 0x75, 0x6E, 0x63, 0x74, 0x69, 0x6F, 0x6E,
    0x00, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x07, 0x70, 0x61, 0x67, 0x65, 0x55, 0x72, 0x6C,
    0x02, 0x00, 0x04, 0x6E, 0x75, 0x6C, 0x6C,

    0x00, 0x0E, 0x6F, 0x62, 0x6A, 0x65, 0x63, 0x74, 0x45, 0x6E, 0x63, 0x6F, 0x64, 0x69, 0x6E, 0x67, 
    0x00, 0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    
    0x00, 0x00, 0x09
  };

  Amf0InvokeMessage *msg = amf0_create_invoke_message();
  amf0_deserialize_invoke_message(msg, input);

  printf("Deserialization\n");
  printf("Command: %s\n", bdata(msg->command));
  printf("Transaction ID: %f\n", msg->transaction_id);
  printf("Arguments:\n");
  print_amf0_object(msg->arguments);
  printf("\n");
  
  amf0_destroy_invoke_message(msg);

  return 1;
error:
  return 0;
}

int test_serialize_real_response_message() {
// (command) “_result”
// (transaction id) 1
// (value)
// [1] {   fmsVer: "FMS/3,5,5,2004"
//         capabilities: 31.0
//         mode: 1.0 },
// [2] {   level: “status”,
//         code: “NetConnection.Connect.Succeeded",
//         description: “Connection succeeded”,
//         data: (array) {
//                version: “3,5,5,2004” },
//         clientId:, 0x1584259571.0,
//         objectEncoding: 3.0 }
  Amf0ResponseMessage *msg = amf0_create_response_message();

  msg->command = bfromcstr("_result");
  msg->transaction_id = 1.0;

  Amf0ObjectValue *val, *sub_val;

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_STRING;
  val->string = bfromcstr("FMS/3,5,5,2004");
  Hashmap_set(msg->properties, bfromcstr("fmsVer"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_NUMBER;
  val->number = 31.0;
  Hashmap_set(msg->properties, bfromcstr("capabilities"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_NUMBER;
  val->number = 1.0;
  Hashmap_set(msg->properties, bfromcstr("mode"), val);


  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_STRING;
  val->string = bfromcstr("status");
  Hashmap_set(msg->information, bfromcstr("level"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_STRING;
  val->string = bfromcstr("NetConnection.Connect.Succeeded");
  Hashmap_set(msg->information, bfromcstr("code"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_STRING;
  val->string = bfromcstr("Connection succeeded");
  Hashmap_set(msg->information, bfromcstr("description"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_ECMA_ARRAY;
  val->ecma_array = Hashmap_create(NULL, NULL);

    sub_val = malloc(sizeof(Amf0ObjectValue));
    sub_val->type = AMF0_STRING;
    sub_val->string = bfromcstr("3,5,5,2004");

    Hashmap_set(val->ecma_array, bfromcstr("version"), sub_val);
  Hashmap_set(msg->information, bfromcstr("data"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_NUMBER;
  val->number = 1584259571.0;
  Hashmap_set(msg->information, bfromcstr("clientId"), val);

  val = malloc(sizeof(Amf0ObjectValue));
  val->type = AMF0_NUMBER;
  val->number = 3.0;
  Hashmap_set(msg->information, bfromcstr("objectEncoding"), val);



  unsigned char buffer[65536];
  int count = amf0_serialize_response_message(buffer, msg);

  printf("length=%d\n", count);
  int i;
  for (i=0;i<count;i++) {
    printf("%02x ", buffer[i]);
  }
  printf("\n\n\n");
}

int test_deserialize_real_response_message()
{
  unsigned char input[] = {
   0x02, 0x00, 0x07, 0x5F, 0x72, 0x65, 0x73, 0x75, 0x6C, 0x74, 0x00, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x06, 0x66, 0x6D, 0x73, 0x56, 0x65, 0x72, 0x02, 0x00, 0x0E, 0x46, 0x4D, 0x53, 0x2F, 0x33, 0x2C, 0x35, 0x2C, 0x35, 0x2C, 0x32, 0x30, 0x30, 0x34, 0x00, 0x0C, 0x63, 0x61, 0x70, 0x61, 0x62, 0x69, 0x6C, 0x69, 0x74, 0x69, 0x65, 0x73, 0x00, 0x40, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x6D, 0x6F, 0x64, 0x65, 0x00, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x03, 0x00, 0x05, 0x6C, 0x65, 0x76, 0x65, 0x6C, 0x02, 0x00, 0x06, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0x00, 0x04, 0x63, 0x6F, 0x64, 0x65, 0x02, 0x00, 0x1D, 0x4E, 0x65, 0x74, 0x43, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x2E, 0x43, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, 0x2E, 0x53, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x00, 0x0B, 0x64, 0x65, 0x73, 0x63, 0x72, 0x69, 0x70, 0x74, 0x69, 0x6F, 0x6E, 0x02, 0x00, 0x15, 0x43, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x73, 0x75, 0x63, 0x63, 0x65, 0x65, 0x64, 0x65, 0x64, 0x2E, 0x00, 0x04, 0x64, 0x61, 0x74, 0x61, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x02, 0x00, 0x0A, 0x33, 0x2C, 0x35, 0x2C, 0x35, 0x2C, 0x32, 0x30, 0x30, 0x34, 0x00, 0x00, 0x09, 0x00, 0x08, 0x63, 0x6C, 0x69, 0x65, 0x6E, 0x74, 0x69, 0x64, 0x00, 0x41, 0xD7, 0x9B, 0x78, 0x7C, 0xC0, 0x00, 0x00, 0x00, 0x0E, 0x6F, 0x62, 0x6A, 0x65, 0x63, 0x74, 0x45, 0x6E, 0x63, 0x6F, 0x64, 0x69, 0x6E, 0x67, 0x00, 0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09
  };

  Amf0ResponseMessage *msg = amf0_create_response_message();
  amf0_deserialize_response_message(msg, input);

  printf("Deserialization\n");
  printf("Command: %s\n", bdata(msg->command));
  printf("Transaction ID: %f\n", msg->transaction_id);
  printf("Properties:\n");
  print_amf0_object(msg->properties);
  printf("Information:\n");
  print_amf0_object(msg->information);
  printf("\n");
  
  amf0_destroy_response_message(msg);

  return 1;
error:
  return 0;
}

char *test_functions()
{
  mu_assert(test_deserialize_real_invoke_message(), "test_deserialize_real_invoke_message() failed.");
  mu_assert(test_serialize_real_response_message(), "test_serialize_real_response_message() failed.");
  mu_assert(test_deserialize_real_response_message(), "test_deserialize_real_response_message() failed.");
  mu_assert(test_serialize_and_deserialize_invoke_message(), "test_serialize_and_deserialize_invoke_message() failed.");

  return NULL;
}


char *all_tests() {
    mu_suite_start();

    mu_run_test(test_functions);

    return NULL;
}

RUN_TESTS(all_tests);