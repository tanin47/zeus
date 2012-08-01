#ifndef __amf0_h__
#define __amf0_h__

#include <basic.h>
#include <lcthw/bstrlib.h>
#include <lcthw/hashmap.h>
#include <dbg.h>

typedef enum { 
  AMF0_UNRECOGNIZED_TYPE,
  AMF0_NUMBER,
  AMF0_BOOLEAN,
  AMF0_STRING,
  AMF0_OBJECT,
  AMF0_MOVIE_CLIP,
  AMF0_NULL,
  AMF0_UNDEFINED,
  AMF0_REFERENCE,
  AMF0_ECMA_ARRAY,
  AMF0_OBJECT_END,
  AMF0_STRICT_ARRAY,
  AMF0_DATE,
  AMF0_LONG_STRING,
  AMF0_UNSUPPORTED,
  AMF0_RECORDSET,
  AMF0_XML_DOCUMENT,
  AMF0_TYPED_OBJECT
} Amf0ValueType;


typedef struct Amf0StrictArray Amf0StrictArray;


typedef struct {
  Amf0ValueType type;
  bstring string;
  double number;
  int boolean;
  unsigned short reference;
  Hashmap *object;
  Hashmap *ecma_array;
  Amf0StrictArray *strict_array;
} Amf0ObjectValue;

typedef struct Amf0StrictArray {
  Amf0ObjectValue **data;
  int length;
};


typedef struct {
  bstring command;
  double transaction_id;
  Hashmap *arguments;
} Amf0InvokeMessage;



Amf0InvokeMessage *amf0_create_invoke_message();
void amf0_destroy_invoke_message(Amf0InvokeMessage *msg);

int amf0_serialize_invoke_message(unsigned char *output, Amf0InvokeMessage *msg);
int amf0_deserialize_invoke_message(Amf0InvokeMessage *msg, unsigned char *input);

int amf0_serialize_number(unsigned char *output, double number);
int amf0_deserialize_number(double *number, unsigned char *input);

int amf0_serialize_boolean(unsigned char *output, int boolean);
int amf0_deserialize_boolean(int *boolean,unsigned char *input);

int amf0_serialize_string(unsigned char *output, bstring str);
int amf0_deserialize_string(bstring *output, unsigned char *input);
int amf0_serialize_string_literal(unsigned char *output, bstring str);
int amf0_deserialize_string_literal(bstring *output, unsigned char *input, int length);

int amf0_serialize_object(unsigned char *output, Hashmap *object);
int amf0_deserialize_object(Hashmap *output, unsigned char *input);
int amf0_serialize_object_value(unsigned char *output, Amf0ObjectValue *val);
int amf0_deserialize_object_value(Amf0ObjectValue *output, unsigned char *input);

void print_amf0_object(Hashmap *object);
void print_amf0_object_value(Amf0ObjectValue *val);

int amf0_serialize_null(unsigned char *output);
int amf0_deserialize_null(unsigned char *input);

int amf0_serialize_undefined(unsigned char *output);
int amf0_deserialize_undefined(unsigned char *input);

int amf0_serialize_reference(unsigned char *output, unsigned short reference);
int amf0_deserialize_reference(unsigned short *reference, unsigned char *input);

int amf0_serialize_ecma_array(unsigned char *output, Hashmap *array);
int amf0_deserialize_ecma_array(Hashmap *output, unsigned char *input);

void print_amf0_ecma_array(Hashmap *array);

int amf0_serialize_strict_array(unsigned char *output, Amf0StrictArray *array);
int amf0_deserialize_strict_array(Amf0StrictArray **array, unsigned char *input);

void print_amf0_strict_array(Amf0StrictArray *array);




void amf0_parse(unsigned char *message, int start, int msg_length);



#endif