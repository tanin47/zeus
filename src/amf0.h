#ifndef __amf0_h__
#define __amf0_h__

#include <basic.h>
#include <lcthw/bstrlib.h>
#include <lcthw/hashmap.h>
#include <dbg.h>

typedef struct {
  bstring command;
  double transaction_id;
  Hashmap *arguments;
} Amf0InvokeMessage;

Amf0InvokeMessage *amf0_create_invoke_message();
void amf0_destroy_invoke_message(Amf0InvokeMessage * msg);

void amf0_parse(unsigned char *message, int start, int msg_length);

int amf0_serialize_string(unsigned char *output, unsigned char *input, int length);
int amf0_deserialize_string(unsigned char *output, unsigned char *input, int start, int length);


#endif