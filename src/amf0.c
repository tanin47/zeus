#include <amf0.h>

Amf0InvokeMessage *amf0_create_invoke_message() {
  Amf0InvokeMessage *msg = malloc(sizeof(Amf0InvokeMessage));
  msg->command = NULL;
  msg->transaction_id = 0.0;
  msg->arguments = Hashmap_create(NULL, NULL);

  return msg;
}

void amf0_destroy_invoke_message(Amf0InvokeMessage *msg) {
  if (msg->command != NULL) bdestroy(msg->command);
  msg->transaction_id = 0.0;
  if (msg->arguments != NULL) Hashmap_destroy(msg->arguments);
}

int amf0_serialize_invoke_message(unsigned char *output, Amf0InvokeMessage *msg) {
  int i = 0;

  i += amf0_serialize_string(output + i, msg->command);
  i += amf0_serialize_number(output + i, msg->transaction_id);
  i += amf0_serialize_object(output + i, msg->arguments);

  return i;
error:
  return -1;
}

int amf0_serialize_object(unsigned char *output, Hashmap *object) {
  int count = 0;
  output[count++] = 0x03;

  int i = 0;
  int j = 0;
  int rc = 0;

  for(i = 0; i < DArray_count(object->buckets); i++) {
      DArray *bucket = DArray_get(object->buckets, i);
      if(bucket) {
          for(j = 0; j < DArray_count(bucket); j++) {
              HashmapNode *node = DArray_get(bucket, j);

              // serialize key
              count += amf0_serialize_string_literal(output + count, node->key);
              count += amf0_serialize_object_value(output + count, (Amf0ObjectValue *)node->data);

              if(rc != 0) return rc;
          }
      }
  }

  output[count++] = 0x00;
  output[count++] = 0x00;
  output[count++] = 0x09;

  return count;

error:
  return -1;
}

int amf0_serialize_object_value(unsigned char *output, Amf0ObjectValue *val) {
  if (val->type == AMF0_NUMBER) {
    return amf0_serialize_number(output, val->number);
  } else if (val->type == AMF0_STRING) {
    return amf0_serialize_string(output, val->string);
  } else {
    return 0;
  }

error:
  return -1;
}

int amf0_serialize_string(unsigned char *output, bstring str) {
  int i = 0;
  output[i++] = 0x02;

  i += amf0_serialize_string_literal(output + i, str);

  return i;
error:
  return -1;
}

int amf0_serialize_string_literal(unsigned char *output, bstring str) {
  int_to_byte_array(str->slen, output, 0, 2);

  int i;
  for (i=0;i<str->slen;i++) {
    output[i + 2] = str->data[i];
  }

  return str->slen + 2;
error:
  return -1;
}

int amf0_serialize_number(unsigned char *output, double number) {
  output[0] = 0x00;
  memcpy(output + 1, &number, 8);

  int i;
  for (i=0;i<4;i++) {
    unsigned char b = output[i+1];
    output[i+1] = output[8-i];
    output[8-i] = b;
  }

  return 9;  
error:
  return -1;
}


void amf0_parse(unsigned char *message, int start, int msg_length) {

  printf("Parse %d %d\n", start, msg_length);

  int i,j;
  for (i=start;i<(start + msg_length);i++) {

    printf("type = %u\n", message[i]);

    if (message[i] == 0x02) {
      int length = chars_to_int(message + i + 1, 2);

      i += 3;
      for (j=0;j<length;j++)
        printf("%c", message[i + j]);
      printf("\n");

      i += length - 1;
    } else if (message[i] == 0x00) {
      double number = chars_to_double(message + i + 1, 8);
      printf("%f\n", number);
      i += 1 + 8 - 1;
    } else if (message[i] == 0x03) {
      printf("Start-object\n");
      int length = chars_to_int(message + i + 1, 2);

      i += 3;
      for (j=0;j<length;j++)
        printf("%c", message[i + j]);
      printf("\n");

      i += length - 1;
      i++;
      length = chars_to_int(message + i + 1, 4);

      i += 5;
      for (j=0;j<length;j++)
        printf("%c", message[i + j]);
      printf("\n");

      i += length - 1;
    } else if (message[i] == 0x09) {
      printf("End-object\n");
    } else if (message[i] == 0x08) {
      printf("End-object\n");
    } else {
      break;
    }
  }
}