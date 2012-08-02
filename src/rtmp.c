#include <rtmp.h>

RtmpOutputMessage *rtmp_create_output_message() {
  RtmpOutputMessage *output = malloc(sizeof(RtmpOutputMessage));
  output->message = NULL;
  output->length = 0;
  output->data_left = 0;

  return output;
}

void rtmp_allocate_output_message_content(RtmpOutputMessage *output, unsigned int length) {
  output->message = malloc(sizeof(unsigned char) * length);
  output->length = length;
  output->data_left = 0;
}

void rtmp_destroy_output_message(RtmpOutputMessage *output) {
  if (output->message != NULL) free(output->message);
  
  output->message = NULL;
  output->length = 0;
  output->data_left = 0;

  free(output);
}

unsigned int rtmp_output_message_start_at(RtmpOutputMessage *output) {
  return output->message + output->length - output->data_left;
}

Rtmp *rtmp_create() {
  Rtmp *rtmp = malloc(sizeof(Rtmp));
  rtmp->state = WAIT_FOR_C0;
  rtmp->chunk_size = 128;
  rtmp->c1 = NULL;
  return rtmp;
}

void rtmp_destroy(Rtmp *rtmp) {
  if (rtmp->c1 != NULL) free(rtmp->c1);
  free(rtmp);
}

int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  while (RingBuffer_available_data(buffer) > 0) {
    printf("----------------------\n");
    printf("Available data: %d, State=%d\n", RingBuffer_available_data(buffer), rtmp->state);

    int retVal = -1;

    if (rtmp->state == WAIT_FOR_C0) {
      retVal = rtmp_process_c0(rtmp, buffer, output);
    } else if (rtmp->state == WAIT_FOR_C1) {
      retVal = rtmp_process_c1(rtmp, buffer, output);
    } else if (rtmp->state == WAIT_FOR_C2) {
      retVal = rtmp_process_c2(rtmp, buffer, output);
    } else if (rtmp->state == READ_CHUNK_TYPE) {
      retVal = rtmp_process_read_chunk_type(rtmp, buffer, output);
    } else if (rtmp->state == READ_CHUNK_EXTENDED_ID) {
      retVal = rtmp_process_read_chunk_extended_id(rtmp, buffer, output);
    } else if (rtmp->state == READ_CHUNK_HEADER) {
      retVal = rtmp_process_read_chunk_header(rtmp, buffer, output);
    } else if (rtmp->state == READ_CHUNK_EXTENDED_TIMESTAMP) {
      retVal = rtmp_process_read_chunk_extended_timestamp(rtmp, buffer, output);
    } else if (rtmp->state == READ_CHUNK_DATA) {
      retVal = rtmp_process_read_chunk_data(rtmp, buffer, output);
    } else if (rtmp->state == READ_NOTHING) {
      retVal = rtmp_process_read_nothing(rtmp, buffer, output);
    } else {
      return -1;
    }

    printf("Data left: %d, State=%d\n", RingBuffer_available_data(buffer), rtmp->state);

    if (retVal == 0) break;
  }

  return 1;
}

int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (RingBuffer_available_data(buffer) < 1) return 0;

  printf("Process C0, Write S0,S1\n");

  rtmp_allocate_output_message_content(output, 1537);
  RingBuffer_read(buffer, output->message, 1);

  int i;
  for (i=1;i<1537;i++) {
    output->message[i] = 0;
  }

  rtmp->state = WAIT_FOR_C1;

  return 1;
}

int rtmp_process_c1(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (RingBuffer_available_data(buffer) < 1536) return 0;

  printf("Process C1, Write S2\n");

  rtmp_allocate_output_message_content(output, 1536);
  RingBuffer_read(buffer, output->message, 1536);

  output->message[4] = 0;
  output->message[5] = 0;
  output->message[6] = 0;
  output->message[7] = 0;

  rtmp->state = WAIT_FOR_C2;

  return 1;
}

int rtmp_process_c2(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (RingBuffer_available_data(buffer) < 1536) return 0;

  printf("Process C2\n");

  unsigned char read[1536];
  RingBuffer_read(buffer, read, 1536);

  rtmp->state = READ_CHUNK_TYPE;

  return 1;
}

int rtmp_process_read_chunk_type(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (RingBuffer_available_data(buffer) < 1) return 0;

  printf("Read chunk type\n");
  rtmp->chunk_type = UNRECOGNIZED_CHUNK_TYPE;

  unsigned char byte;
  RingBuffer_read(buffer, &byte, 1);
  rtmp->chunk_id = (unsigned int) byte;

  unsigned char first = rtmp->chunk_id & 0b10000000;
  unsigned char second = rtmp->chunk_id & 0b01000000;

  rtmp->chunk_id = rtmp->chunk_id & 0b00111111;

  if (first == 0 && second == 0) {
    printf("Chunk Type 0\n");
    rtmp->chunk_type = TYPE_0;
  } else if (first == 0 && second > 0) {
    printf("Chunk Type 1\n");
    rtmp->chunk_type = TYPE_1;
  } else if (first > 0 && second == 0) {
    printf("Chunk Type 2\n");
    rtmp->chunk_type = TYPE_2;
  } else if (first > 0 && second > 0) {
    printf("Chunk Type 3\n");
    rtmp->chunk_type = TYPE_3;
  } else {
    printf("Unrecognized chunk type %u %u\n", first, second);
    rtmp->chunk_type = UNRECOGNIZED_CHUNK_TYPE;
  }

  printf("Chunk ID: %u\n", rtmp->chunk_id);
  if (rtmp->chunk_id <= 2) {
    rtmp->state = READ_CHUNK_EXTENDED_ID;
  } else {
    rtmp->state = READ_CHUNK_HEADER;
  }

  return 1;
}

int rtmp_process_read_chunk_extended_id(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (rtmp->chunk_id == 0 && RingBuffer_available_data(buffer) < 1) return 0;
  if (rtmp->chunk_id == 1 && RingBuffer_available_data(buffer) < 2) return 0;

  if (rtmp->chunk_id == 0) {
    unsigned char byte;
    RingBuffer_read(buffer, &byte, 1);
    rtmp->chunk_id = byte + 64;
  } else if (rtmp->chunk_id == 1) {
    unsigned char bytes[2];
    RingBuffer_read(buffer, bytes, 2);
    rtmp->chunk_id = (bytes[1] * 256) + bytes[0] + 64;
  } else if (rtmp->chunk_id == 2) {

  } else {
    printf("Unrecognized extended id: %d\n", rtmp->chunk_id);
    exit(1);
  }

  printf("Chunk ID: %u\n", rtmp->chunk_id);
  rtmp->state = READ_CHUNK_HEADER;

  return 1;
}

int rtmp_process_read_chunk_header(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (rtmp->chunk_type == TYPE_0 && RingBuffer_available_data(buffer) < 11) return 0;
  else if (rtmp->chunk_type == TYPE_1 && RingBuffer_available_data(buffer) < 7) return 0;
  else if (rtmp->chunk_type == TYPE_2 && RingBuffer_available_data(buffer) < 3) return 0;
  else if (rtmp->chunk_type == TYPE_3) {
    rtmp->state = READ_CHUNK_DATA;
    return 1;
  } else {
    printf("Unknown chunk type\n");
    exit(1);
  }
  
  unsigned char timestamp[3];
  unsigned char message_length[3];
  unsigned char message_type[1];
  unsigned char message_stream_id[4];

  RingBuffer_read(buffer, timestamp, 3);
  rtmp->chunk_timestamp = chars_to_int(timestamp, 3);

  if (rtmp->chunk_type == TYPE_0 || rtmp->chunk_type == TYPE_1) {
    RingBuffer_read(buffer, message_length, 3);
    rtmp->message_length = chars_to_int(message_length, 3);

    RingBuffer_read(buffer, message_type, 1);
    rtmp->message_type = chars_to_int(message_type, 1);
  }

  if (rtmp->chunk_type == TYPE_0) {
    RingBuffer_read(buffer, message_stream_id, 4);
    rtmp->message_stream_id = chars_to_int(message_stream_id, 4);
  }

  printf("Read header type 0\n");
  printf("Timestamp=%u, message_length=%u, message_type=%u, message_stream_id=%u\n", 
          rtmp->chunk_timestamp,
          rtmp->message_length,
          rtmp->message_type,
          rtmp->message_stream_id);

  rtmp_read_extended_timestamp_or_data(rtmp);
  return 1;
}

void rtmp_read_extended_timestamp_or_data(Rtmp *rtmp) {
  if (rtmp->chunk_timestamp == 0xffffff) {
    rtmp->state = READ_CHUNK_EXTENDED_TIMESTAMP;
  } else {    
    rtmp->message = malloc(sizeof(char) * rtmp->message_length);
    rtmp->message_data_left = rtmp->message_length;
    rtmp->state = READ_CHUNK_DATA;
  }
}


int rtmp_process_read_chunk_extended_timestamp(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (RingBuffer_available_data(buffer) < 4) return 0;

  printf("Read extended timestamp.\n");

  char extended_timestamp[4];
  RingBuffer_read(buffer, extended_timestamp, 4);

  rtmp->chunk_timestamp = chars_to_int(timestamp, 4);
  rtmp->state = READ_CHUNK_DATA;

  return 1;
}

int rtmp_process_read_chunk_data(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  int read_amount = rtmp->message_data_left;
  if (rtmp->chunk_size < read_amount) 
    read_amount = rtmp->chunk_size;

  if (RingBuffer_available_data(buffer) < read_amount) return 0;

  printf("Read data: %d, left: %d\n",rtmp->message, rtmp->message_data_left);

  RingBuffer_read(buffer, rtmp->message + rtmp->message_length - rtmp->message_data_left, read_amount);
  rtmp->message_data_left -= read_amount;

  printf("Finish read data: %d\n", read_amount);
  rtmp->state = READ_CHUNK_TYPE;

  if (rtmp->message_data_left == 0) {
    //propagate to top
    Amf0InvokeMessage *invoke = amf0_create_invoke_message();
    amf0_deserialize_invoke_message(invoke, rtmp->message);

    printf("Command: %s\n", bdata(invoke->command));
    printf("Transaction ID: %f\n", invoke->transaction_id);
    printf("Arguments:\n");
    print_amf0_object(invoke->arguments);
    printf("\n");
    
    amf0_destroy_invoke_message(invoke);

    Amf0ResponseMessage *msg = amf0_create_response_message();

    msg->command = bfromcstr("_result");
    msg->transaction_id = 1.0;
    msg->properties = Hashmap_create(NULL, NULL);
    msg->information = Hashmap_create(NULL, NULL);

    Amf0ObjectValue *val, *sub_val;

    val = malloc(sizeof(Amf0ObjectValue));
    val->type = AMF0_STRING;
    val->string = bfromcstr("FMS/3,5,5,2004");
    Hashmap_set(msg->properties, bfromcstr("fmsVer"), val);

    val = malloc(sizeof(Amf0ObjectValue));
    val->type = AMF0_NUMBER;
    val->number = 239.000000;
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
    val->string = bfromcstr("NetConnection.Connect.Success");
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
      sub_val->string = bfromcstr("zeus.0.0.1.tanin");

      Hashmap_set(val->ecma_array, bfromcstr("version"), sub_val);
    Hashmap_set(msg->information, bfromcstr("data"), val);

    val = malloc(sizeof(Amf0ObjectValue));
    val->type = AMF0_NUMBER;
    val->number = 1584259571.0;
    Hashmap_set(msg->information, bfromcstr("clientId"), val);

    val = malloc(sizeof(Amf0ObjectValue));
    val->type = AMF0_NUMBER;
    val->number = 0.0;
    Hashmap_set(msg->information, bfromcstr("objectEncoding"), val);

    printf("Command: %s\n", bdata(msg->command));
    printf("Transaction ID: %f\n", msg->transaction_id);
    printf("Properties:\n");
    print_amf0_object(msg->properties);
    printf("Information:\n");
    print_amf0_object(msg->information);
    printf("\n");

    unsigned char write[65536];
    int count = amf0_serialize_response_message(write, msg);
    printf("count=%d\n", count);

    amf0_destroy_response_message(msg);

    unsigned char header[12];
    header[0] = 0x03;

    header[1] = 0x00;
    header[2] = 0x00;
    header[3] = 0x00;

    // length
    int_to_byte_array(count, header, 4, 3);
    // printf("%x %x %x\n", header[4], header[5], header[6]);

    header[7] = 0x14;

    header[8] = 0x00;
    header[9] = 0x00;  
    header[10] = 0x00;
    header[11] = 0x00;

    rtmp_allocate_output_message_content(output, 12 + 1 + 1 + count);

    int p = 0;
    memcpy(output->message, header, p); p += 12;
    memcpy(output->message + p, write, 128); p += 128;

    unsigned char chunk_header = 0b11000011;
    memcpy(output->message + p, &chunk_header, 1); p += 1;
    memcpy(output->message + p, write + 128, 128); p += 128;

    memcpy(output->message + p, &chunk_header, 1); p += 1;
    memcpy(output->message + p, write + 128 + 128, count - 128 - 128); p += count - 128 - 128;

    rtmp->state = READ_NOTHING;
  }

  return 1;
}

int rtmp_process_read_nothing(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (RingBuffer_available_data(buffer) == 0) return 0;

  int length = RingBuffer_available_data(buffer);

  unsigned char data[length];
  RingBuffer_read(buffer, data, length);

  int i;
  for (i=0;i<length;i++) {
    printf("0x%02x, ", data[i]);
  }
  printf("\n");

  return 1;
}