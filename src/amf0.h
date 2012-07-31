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

typedef struct {
  Amf0ValueType type;
  bstring string;
  double number;
  int boolean;
} Amf0ObjectValue;

typedef struct {
  bstring command;
  double transaction_id;
  Hashmap *arguments;
} Amf0InvokeMessage;


Amf0InvokeMessage *amf0_create_invoke_message();
void amf0_destroy_invoke_message(Amf0InvokeMessage *msg);

int amf0_serialize_invoke_message(unsigned char *output, Amf0InvokeMessage *msg);

int amf0_serialize_object(unsigned char *output, Hashmap *object);
int amf0_serialize_object_value(unsigned char *output, Amf0ObjectValue *val);

int amf0_serialize_string(unsigned char *output, bstring str);
int amf0_serialize_string_literal(unsigned char *output, bstring str);
int amf0_serialize_number(unsigned char *output, double number);





void amf0_parse(unsigned char *message, int start, int msg_length);



#endif