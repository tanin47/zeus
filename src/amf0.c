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

int amf0_serialize_string(unsigned char *output, unsigned char *input, int str_length) {
  output[0] = 0x02;
  int_to_byte_array(str_length, output, 1, 2);

  int i;
  for (i=0;i<str_length;i++) {
    output[i + 3] = input[i];
  }

  return 0;
error:
  return -1;
}

int amf0_deserialize_string(unsigned char *output, unsigned char *input, int start, int length) {
  int i;
  for (i=0;i<(length -3);i++) {
    output[i] = input[start + i + 3];
  }

  return 0;  
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