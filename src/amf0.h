#ifndef __amf0_h__
#define __amf0_h__

#include <basic.h>
#include <lcthw/bstrlib.h>
#include <lcthw/hashmap.h>
#include <dbg.h>

typedef enum { 
  AMF0_NUMBER = 0x00,
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
  bstring class_name;
  Hashmap *object;
} Amf0TypedObject;

typedef struct {
  Amf0ValueType type;
  bstring string;
  double number;
  double date;
  int boolean;
  unsigned short reference;
  Hashmap *object;
  Amf0TypedObject *typed_object;
  Hashmap *ecma_array;
  Amf0StrictArray *strict_array;
  bstring xml_document;
} Amf0ObjectValue;

struct Amf0StrictArray {
  Amf0ObjectValue **data;
  int length;
};

typedef struct {
  bstring command;
  bstring description;
} Amf0GenericInvokeMessage;

Amf0GenericInvokeMessage *amf0_create_generic_invoke_message();
void amf0_destroy_generic_invoke_message(Amf0GenericInvokeMessage *msg);

int amf0_serialize_generic_invoke_message(unsigned char *output, Amf0GenericInvokeMessage *msg);
int amf0_deserialize_generic_invoke_message(Amf0GenericInvokeMessage *msg, unsigned char *input);




typedef struct {
  bstring command;
  double transaction_id;
  Hashmap *arguments;
} Amf0ConnectMessage;

typedef struct {
  bstring command;
  double transaction_id;
  Hashmap *properties;
  Hashmap *information;
} Amf0ResponseConnectMessage;

Amf0ConnectMessage *amf0_create_connect_message();
void amf0_destroy_connect_message(Amf0ConnectMessage *msg);

Amf0ResponseConnectMessage *amf0_create_response_connect_message();
void amf0_destroy_response_connect_message(Amf0ResponseConnectMessage *msg);

int amf0_serialize_connect_message(unsigned char *output, Amf0ConnectMessage *msg);
int amf0_deserialize_connect_message(Amf0ConnectMessage *msg, unsigned char *input);

int amf0_serialize_response_connect_message(unsigned char *output, Amf0ResponseConnectMessage *msg);
int amf0_deserialize_response_connect_message(Amf0ResponseConnectMessage *msg, unsigned char *input);



typedef struct {
  bstring command;
  double transaction_id;
  Hashmap *object;
} Amf0CreateStreamMessage;

typedef struct {
  bstring command;
  double transaction_id;
  Hashmap *object;
  double stream_id;
} Amf0ResponseCreateStreamMessage;

Amf0CreateStreamMessage *amf0_create_create_stream_message();
void amf0_destroy_create_stream_message(Amf0CreateStreamMessage *msg);

Amf0ResponseCreateStreamMessage *amf0_create_response_create_stream_message();
void amf0_destroy_response_create_stream_message(Amf0ResponseCreateStreamMessage *msg);

int amf0_serialize_create_stream_message(unsigned char *output, Amf0CreateStreamMessage *msg);
int amf0_deserialize_create_stream_message(Amf0CreateStreamMessage *msg, unsigned char *input);

int amf0_serialize_response_create_stream_message(unsigned char *output, Amf0ResponseCreateStreamMessage *msg);
int amf0_deserialize_response_create_stream_message(Amf0ResponseCreateStreamMessage *msg, unsigned char *input);


typedef struct {
  bstring command;
  double transaction_id;
  bstring publishing_name;
  bstring publishing_type;
} Amf0PublishMessage;

Amf0PublishMessage *amf0_create_publish_message();
void amf0_destroy_publish_message(Amf0PublishMessage *msg);

int amf0_serialize_publish_message(unsigned char *output, Amf0PublishMessage *msg);
int amf0_deserialize_publish_message(Amf0PublishMessage *msg, unsigned char *input);



void amf0_destroy_object(Hashmap *object);
void amf0_destroy_object_value(Amf0ObjectValue *object);
void amf0_destroy_typed_object(Amf0TypedObject *object);
void amf0_destroy_ecma_array(Hashmap *object);
void amf0_destroy_strict_array(Amf0StrictArray *strict_array);

int amf0_serialize_number(unsigned char *output, double number);
int amf0_deserialize_number(double *number, unsigned char *input);

int amf0_serialize_boolean(unsigned char *output, int boolean);
int amf0_deserialize_boolean(int *boolean,unsigned char *input);

int amf0_serialize_string(unsigned char *output, bstring str);
int amf0_deserialize_string(bstring *output, unsigned char *input);
int amf0_serialize_string_literal(unsigned char *output, bstring str, int length_of_length);
int amf0_deserialize_string_literal(bstring *output, unsigned char *input, int length);

int amf0_serialize_object(unsigned char *output, Hashmap *object);
int amf0_serialize_object_content(unsigned char *output, Hashmap *object);
int amf0_deserialize_object(Hashmap **output_pointer, unsigned char *input);
int amf0_serialize_object_value(unsigned char *output, Amf0ObjectValue *val);
int amf0_deserialize_object_value(Amf0ObjectValue **output_pointer, unsigned char *input);

void print_amf0_object(Hashmap *object);
void print_amf0_object_value(Amf0ObjectValue *val);

int amf0_serialize_movie_clip(unsigned char *output);
int amf0_deserialize_movie_clip(unsigned char *input);

int amf0_serialize_null(unsigned char *output);
int amf0_deserialize_null(unsigned char *input);

int amf0_serialize_undefined(unsigned char *output);
int amf0_deserialize_undefined(unsigned char *input);

int amf0_serialize_reference(unsigned char *output, unsigned short reference);
int amf0_deserialize_reference(unsigned short *reference, unsigned char *input);

int amf0_serialize_ecma_array(unsigned char *output, Hashmap *array);
int amf0_deserialize_ecma_array(Hashmap **output_pointer, unsigned char *input);

void print_amf0_ecma_array(Hashmap *array);

int amf0_serialize_strict_array(unsigned char *output, Amf0StrictArray *array);
int amf0_deserialize_strict_array(Amf0StrictArray **array, unsigned char *input);

void print_amf0_strict_array(Amf0StrictArray *array);

int amf0_serialize_date(unsigned char *output, double date);
int amf0_deserialize_date(double *date, unsigned char *input);

int amf0_serialize_long_string(unsigned char *output, bstring str);
int amf0_deserialize_long_string(bstring *output, unsigned char *input);

int amf0_serialize_unsupported(unsigned char *output);
int amf0_deserialize_unsupported(unsigned char *input);

int amf0_serialize_record_set(unsigned char *output);
int amf0_deserialize_record_set(unsigned char *input);

int amf0_serialize_xml_document(unsigned char *output, bstring str);
int amf0_deserialize_xml_document(bstring *output, unsigned char *input);

int amf0_serialize_typed_object(unsigned char *output, Amf0TypedObject *object);
int amf0_deserialize_typed_object(Amf0TypedObject **output_pointer, unsigned char *input);

void amf0_parse(unsigned char *message, int start, int msg_length);



#endif