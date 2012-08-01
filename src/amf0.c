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

int amf0_deserialize_invoke_message(Amf0InvokeMessage *msg, unsigned char *input) {
  unsigned int start = input;

  input++;
  input += amf0_deserialize_string(&(msg->command), input);

  input++;
  input += amf0_deserialize_number(&(msg->transaction_id), input);

  input++;
  input += amf0_deserialize_object(msg->arguments, input);

  return (input - start + 1);
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

int amf0_deserialize_number(double *number, unsigned char *input) {
  int i;
  for (i=0;i<8;i++) {
    memcpy((char *)number + i, input + 7 - i, 1);
  }

  return 8;  
error:
  return -1;
}

int amf0_serialize_boolean(unsigned char *output, int boolean) {
  output[0] = 0x01;
  output[1] = (boolean == 0) ? 0x00 : 0x01;

  return 2;  
error:
  return -1;
}

int amf0_deserialize_boolean(int *boolean, unsigned char *input) {
  (*boolean) = (input[0] == 0) ? 0 : 1;
  return 1;  
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

int amf0_deserialize_string(bstring *output, unsigned char *input) {
  int length = chars_to_int(input, 2);

  int check = amf0_deserialize_string_literal(output, input + 2, length);
  if (check == -1) return -1;

  return 2 + length;
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

int amf0_deserialize_string_literal(bstring *output, unsigned char *input, int length) {
  unsigned char tmp[length + 1];

  memcpy(tmp, input, length);
  tmp[length] = '\0';

  bstring str = bfromcstr(tmp);
  (*output) = str;
  return length;
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

int amf0_deserialize_object(Hashmap *output, unsigned char *input) {
  unsigned int start = input;

  while (!(input[0] == 0x00 && input[1] == 0x00 && input[2] == 0x09)) {
    bstring key;
    Amf0ObjectValue *value = malloc(sizeof(Amf0ObjectValue));

    input += amf0_deserialize_string(&key, input);
    input += amf0_deserialize_object_value(value, input);

    Hashmap_set(output, key, value);
  }

  return input + 3 - start;
error:
  return -1;
}

int amf0_serialize_object_value(unsigned char *output, Amf0ObjectValue *val) {
  if (val->type == AMF0_NUMBER) {
    return amf0_serialize_number(output, val->number);
  } else if (val->type == AMF0_BOOLEAN) {
    return amf0_serialize_boolean(output, val->boolean);
  } else if (val->type == AMF0_STRING) {
    return amf0_serialize_string(output, val->string);
  } else if (val->type == AMF0_OBJECT) {
    return amf0_serialize_object(output, val->object);
  } else if (val->type == AMF0_NULL) {
    return amf0_serialize_null(output);
  } else if (val->type == AMF0_MOVIE_CLIP) {
    printf("movie-clip is not supported\n");
    exit(1);
  } else if (val->type == AMF0_UNDEFINED) {
    return amf0_serialize_undefined(output);
  } else if (val->type == AMF0_REFERENCE) {
    return amf0_serialize_reference(output, val->reference);
  } else if (val->type == AMF0_ECMA_ARRAY) {
    return amf0_serialize_ecma_array(output, val->ecma_array);
  } else if (val->type == AMF0_STRICT_ARRAY) {
    return amf0_serialize_strict_array(output, val->strict_array);
  } else {
    return 0;
  }

error:
  return -1;
}

int amf0_deserialize_object_value(Amf0ObjectValue *output, unsigned char *input) {
  unsigned int start_data = input + 1;
  int count = 0;

  printf("%02x ", input[0]);

  if (input[0] == 0x00) {
    output->type = AMF0_NUMBER;
    count = amf0_deserialize_number(&(output->number), start_data);
  } else if (input[0] == 0x01) {
    output->type = AMF0_BOOLEAN;
    count = amf0_deserialize_boolean(&(output->boolean), start_data);
  } else if (input[0] == 0x02) {
    output->type = AMF0_STRING;
    count = amf0_deserialize_string(&(output->string), start_data);
  } else if (input[0] == 0x03) {
    output->type = AMF0_OBJECT;
    output->object = Hashmap_create(NULL, NULL);
    count = amf0_deserialize_object(output->object, start_data);
  } else if (input[0] == 0x04) {
    output->type = AMF0_NULL;
    count = amf0_deserialize_null(start_data);
  } else if (input[0] == 0x05) {
    output->type = AMF0_MOVIE_CLIP;
    printf("movie-clip is not supported\n");
    exit(1);
  } else if (input[0] == 0x06) {
    output->type = AMF0_UNDEFINED;
    count = amf0_deserialize_undefined(start_data);
  } else if (input[0] == 0x07) {
    output->type = AMF0_REFERENCE;
    count = amf0_deserialize_reference(&(output->reference), start_data);
  } else if (input[0] == 0x08) {
    output->type = AMF0_ECMA_ARRAY;
    output->ecma_array = Hashmap_create(NULL, NULL);
    count = amf0_deserialize_ecma_array(output->ecma_array, start_data);
  } else if (input[0] == 0x09) {
    printf("wtf! this is object-end-marker\n");
    exit(1);
  } else if (input[0] == 0x0A) {
    output->type = AMF0_STRICT_ARRAY;
    count = amf0_deserialize_strict_array(&(output->strict_array), start_data);
  } else {
    count = 0;
  }

  return count + 1;
error:
  return -1;
}


void print_amf0_object(Hashmap *object) {
  int i = 0;
  int j = 0;
  int rc = 0;

  for(i = 0; i < DArray_count(object->buckets); i++) {
      DArray *bucket = DArray_get(object->buckets, i);
      if(bucket) {
          for(j = 0; j < DArray_count(bucket); j++) {
              HashmapNode *node = DArray_get(bucket, j);

              printf("\t%s -> ", bdata((bstring)(node->key)));
              print_amf0_object_value(node->data);
              printf("\n");


              if(rc != 0) return rc;
          }
      }
  }
}

void print_amf0_object_value(Amf0ObjectValue *val) {
  if (val->type == AMF0_NUMBER) {
    printf("%f", val->number);
  } else if (val->type == AMF0_BOOLEAN) {
    printf("%d", val->boolean);
  } else if (val->type == AMF0_STRING) {
    printf("%s", bdata(val->string));
  } else if (val->type == AMF0_OBJECT) {
    print_amf0_object(val->object);
  } else if (val->type == AMF0_NULL) {
    printf("<null>");
  } else if (val->type == AMF0_MOVIE_CLIP) {
    printf("<movie_clip>");
  } else if (val->type == AMF0_UNDEFINED) {
    printf("<undefined>");
  } else if (val->type == AMF0_REFERENCE) {
    printf("%d", val->reference);
  } else if (val->type == AMF0_ECMA_ARRAY) {
    print_amf0_ecma_array(val->ecma_array);
  } else if (val->type == AMF0_STRICT_ARRAY) {
    print_amf0_strict_array(val->strict_array);
  } else {
    printf("Unknown");
  }
}


int amf0_serialize_null(unsigned char *output) {
  output[0] = 0x04;
  return 1;
}

int amf0_deserialize_null(unsigned char *input) {
  return 0;
}


int amf0_serialize_undefined(unsigned char *output) {
  output[0] = 0x06;
  return 1;
}

int amf0_deserialize_undefined(unsigned char *input) {
  return 0;
}


int amf0_serialize_reference(unsigned char *output, unsigned short reference) {
  output[0] = 0x07;
  output[1] = ((reference & 0xFF00) >> 8);
  output[2] = (reference & 0x00FF);

  printf("%x %x %x\n", output[1], output[2], reference);
  return 3;
}

int amf0_deserialize_reference(unsigned short *reference, unsigned char *input) {
  *reference = ((unsigned short)(input[0]) << 8) | (unsigned short)input[1];
  return 2;
}

int amf0_serialize_ecma_array(unsigned char *output, Hashmap *array) {
  int count = 0;
  output[count++] = 0x08;

  // length
  output[count++] = 0x00;
  output[count++] = 0x00;
  output[count++] = 0x00;
  output[count++] = 0x00;

  int i = 0;
  int j = 0;
  int rc = 0;

  for(i = 0; i < DArray_count(array->buckets); i++) {
      DArray *bucket = DArray_get(array->buckets, i);
      if(bucket) {
          for(j = 0; j < DArray_count(bucket); j++) {
              HashmapNode *node = DArray_get(bucket, j);

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
}

int amf0_deserialize_ecma_array(Hashmap *output, unsigned char *input) {
  unsigned int start = input;
  input += 4; // skip length (4 bytes)

  while (!(input[0] == 0x00 && input[1] == 0x00 && input[2] == 0x09)) {
    bstring key;
    Amf0ObjectValue *value = malloc(sizeof(Amf0ObjectValue));

    input += amf0_deserialize_string(&key, input);
    input += amf0_deserialize_object_value(value, input);

    Hashmap_set(output, key, value);
  }

  return input + 3 - start;
error:
  return -1;
}

void print_amf0_ecma_array(Hashmap *array) {
  print_amf0_object(array);
}


int amf0_serialize_strict_array(unsigned char *output, Amf0StrictArray *array) {
  int count = 0;

  output[count++] = 0x0A;

  int_to_byte_array(array->length, output, count, count + 3);
  count += 4;

  int i;
  for (i=0;i<array->length;i++) {
    count += amf0_serialize_object_value(output + count, array->data[i]);
  }

  return count;
}

int amf0_deserialize_strict_array(Amf0StrictArray **array, unsigned char *input) {
  int count = 0;

  int length = chars_to_int(input, 4);
  count += 4;

  (*array)->length = length;
  (*array)->data = calloc(length, sizeof(Amf0StrictArray *));

  int i;
  for (i=0;i<length;i++) {
    
  }


  return count;
}

void print_amf0_strict_array(Amf0StrictArray *array) {
  printf("[");
  int i;
  for (i=0;i<array->length;i++) {
    if (i > 0) printf(", ");
    print_amf0_object_value(array->data[i]);
  }
  printf("]");
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