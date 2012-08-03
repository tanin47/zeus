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
  output->data_left = length;
}

void rtmp_destroy_output_message(RtmpOutputMessage *output) {
  if (output->message != NULL) free(output->message);
  
  output->message = NULL;
  output->length = 0;
  output->data_left = 0;

  free(output);
}

unsigned char *rtmp_output_message_start_at(RtmpOutputMessage *output) {
  return output->message + output->length - output->data_left;
}


int rtmp_should_acknowledge(Rtmp *rtmp) {
  if ((rtmp->total_read - rtmp->last_total_read) >= rtmp->window_acknowledgement_size) {
    return 1;
  } else {
    return 0;
  }
}

Rtmp *rtmp_create() {
  Rtmp *rtmp = malloc(sizeof(Rtmp));
  rtmp->chunk_state = WAIT_FOR_C0;
  rtmp->state = WAIT_FOR_CONNECT;
  rtmp->chunk_size = 128;
  rtmp->end = 0;
  rtmp->file = NULL;
  rtmp->last_total_read = 0;
  rtmp->total_read = 0;
  rtmp->window_acknowledgement_size = 2500000;
  return rtmp;
}

void rtmp_destroy(Rtmp *rtmp) {
  free(rtmp);
}

int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  //printf("----------------------\n");
  //printf("Available data: %d, State=%d\n", RingBuffer_available_data(buffer), rtmp->chunk_state);

  int retVal = -1;

  if (rtmp->chunk_state == WAIT_FOR_C0) {
    retVal = rtmp_process_c0(rtmp, buffer, output);
  } else if (rtmp->chunk_state == WAIT_FOR_C1) {
    retVal = rtmp_process_c1(rtmp, buffer, output);
  } else if (rtmp->chunk_state == WAIT_FOR_C2) {
    retVal = rtmp_process_c2(rtmp, buffer, output);
  } else if (rtmp->chunk_state == READ_CHUNK_TYPE) {
    retVal = rtmp_process_read_chunk_type(rtmp, buffer, output);
  } else if (rtmp->chunk_state == READ_CHUNK_EXTENDED_ID) {
    retVal = rtmp_process_read_chunk_extended_id(rtmp, buffer, output);
  } else if (rtmp->chunk_state == READ_CHUNK_HEADER) {
    retVal = rtmp_process_read_chunk_header(rtmp, buffer, output);
  } else if (rtmp->chunk_state == READ_CHUNK_EXTENDED_TIMESTAMP) {
    retVal = rtmp_process_read_chunk_extended_timestamp(rtmp, buffer, output);
  } else if (rtmp->chunk_state == READ_CHUNK_DATA) {
    retVal = rtmp_process_read_chunk_data(rtmp, buffer, output);
  } else if (rtmp->chunk_state == READ_NOTHING) {
    retVal = rtmp_process_read_nothing(rtmp, buffer, output);
  } else {
    return -1;
  }

  //printf("Data left: %d, State=%d\n", RingBuffer_available_data(buffer), rtmp->chunk_state);

  return retVal;
}

int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (RingBuffer_available_data(buffer) < 1) return 0;

  //printf("Process C0, Write S0,S1\n");

  rtmp_allocate_output_message_content(output, 1537);
  RingBuffer_read(buffer, output->message, 1);

  int i;
  for (i=1;i<1537;i++) {
    output->message[i] = 0;
  }

  rtmp->chunk_state = WAIT_FOR_C1;

  return 1;
}

int rtmp_process_c1(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  if (RingBuffer_available_data(buffer) < 1536) return 0;

  //printf("Process C1, Write S2\n");

  rtmp_allocate_output_message_content(output, 1536);
  RingBuffer_read(buffer, output->message, 1536);

  output->message[4] = 0;
  output->message[5] = 0;
  output->message[6] = 0;
  output->message[7] = 0;

  rtmp->chunk_state = WAIT_FOR_C2;

  return 1;
}

int rtmp_process_c2(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  (void) output;

  if (RingBuffer_available_data(buffer) < 1536) return 0;

  //printf("Process C2\n");

  unsigned char read[1536];
  RingBuffer_read(buffer, read, 1536);

  rtmp->chunk_state = READ_CHUNK_TYPE;

  return 1;
}

int rtmp_process_read_chunk_type(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  (void) output;

  if (RingBuffer_available_data(buffer) < 1) return 0;

  //printf("Read chunk type\n");
  rtmp->chunk_type = UNRECOGNIZED_CHUNK_TYPE;

  unsigned char byte;
  RingBuffer_read(buffer, &byte, 1);
  rtmp->chunk_id = (unsigned int) byte;

  unsigned char first = rtmp->chunk_id & 0b10000000;
  unsigned char second = rtmp->chunk_id & 0b01000000;

  rtmp->chunk_id = rtmp->chunk_id & 0b00111111;

  if (first == 0 && second == 0) {
    //printf("Chunk Type 0\n");
    rtmp->chunk_type = TYPE_0;
  } else if (first == 0 && second > 0) {
    //printf("Chunk Type 1\n");
    rtmp->chunk_type = TYPE_1;
  } else if (first > 0 && second == 0) {
    //printf("Chunk Type 2\n");
    rtmp->chunk_type = TYPE_2;
  } else if (first > 0 && second > 0) {
    //printf("Chunk Type 3\n");
    rtmp->chunk_type = TYPE_3;
  } else {
    //printf("Unrecognized chunk type %u %u\n", first, second);
    rtmp->chunk_type = UNRECOGNIZED_CHUNK_TYPE;
  }

  //printf("Chunk ID: %u\n", rtmp->chunk_id);
  if (rtmp->chunk_id <= 2) {
    rtmp->chunk_state = READ_CHUNK_EXTENDED_ID;
  } else {
    rtmp->chunk_state = READ_CHUNK_HEADER;
  }

  return 1;
}

int rtmp_process_read_chunk_extended_id(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  (void) output;

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
    //printf("Unrecognized extended id: %d\n", rtmp->chunk_id);
    exit(1);
  }

  //printf("Chunk ID: %u\n", rtmp->chunk_id);
  rtmp->chunk_state = READ_CHUNK_HEADER;

  return 1;
}

int rtmp_process_read_chunk_header(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  (void) output;

  if (rtmp->chunk_type == TYPE_0) {
    if (RingBuffer_available_data(buffer) < 11) return 0;
  } else if (rtmp->chunk_type == TYPE_1) {
    if (RingBuffer_available_data(buffer) < 7) return 0;
  } else if (rtmp->chunk_type == TYPE_2) {
    if (RingBuffer_available_data(buffer) < 3) return 0;
  } else if (rtmp->chunk_type == TYPE_3) {
    rtmp->chunk_state = READ_CHUNK_DATA;
    return 1;
  } else {
    //printf("Unknown chunk type\n");
    exit(1);
  }
  
  unsigned char timestamp[3];
  unsigned char message_length[3];
  unsigned char message_type[1];
  unsigned char message_stream_id[4];

  RingBuffer_read(buffer, timestamp, 3);

  if (rtmp->chunk_type == TYPE_0) {
    rtmp->chunk_timestamp = chars_to_int(timestamp, 3);
  } else {
    rtmp->chunk_timestamp += chars_to_int(timestamp, 3);
  }

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

  //printf("Read header type 0\n");
  // printf("Timestamp=%u, message_length=%u, message_type=%u, message_stream_id=%u\n", 
          // rtmp->chunk_timestamp,
          // rtmp->message_length,
          // rtmp->message_type,
          // rtmp->message_stream_id);

  rtmp_read_extended_timestamp_or_data(rtmp);
  return 1;
}

void rtmp_read_extended_timestamp_or_data(Rtmp *rtmp) {
  if (rtmp->chunk_timestamp == 0xffffff) {
    rtmp->chunk_state = READ_CHUNK_EXTENDED_TIMESTAMP;
  } else {    
    rtmp->message = malloc(sizeof(unsigned char) * rtmp->message_length);
    rtmp->message_data_left = rtmp->message_length;
    rtmp->chunk_state = READ_CHUNK_DATA;
  }
}


int rtmp_process_read_chunk_extended_timestamp(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  (void) output;
  if (RingBuffer_available_data(buffer) < 4) return 0;

  //printf("Read extended timestamp.\n");

  unsigned char extended_timestamp[4];
  RingBuffer_read(buffer, extended_timestamp, 4);

  rtmp->chunk_timestamp = chars_to_int(extended_timestamp, 4);
  rtmp->chunk_state = READ_CHUNK_DATA;

  return 1;
}

int rtmp_process_read_chunk_data(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  unsigned int read_amount = rtmp->message_data_left;
  if (rtmp->chunk_size < read_amount) 
    read_amount = rtmp->chunk_size;

  if (RingBuffer_available_data(buffer) < (int)read_amount) return 0;

  //printf("Read data: %u, left: %u\n", (unsigned int)rtmp->message, rtmp->message_data_left);

  RingBuffer_read(buffer, rtmp->message + rtmp->message_length - rtmp->message_data_left, read_amount);
  rtmp->message_data_left -= read_amount;

  //printf("Finish read data: %u\n", read_amount);
  rtmp->chunk_state = READ_CHUNK_TYPE;

  if (rtmp->message_data_left == 0) {
    RtmpOutputMessage *this_output = rtmp_create_output_message();
    rtmp_process_message(rtmp, this_output);

    if (this_output->length > 0) {
      int number_of_chunks = this_output->length / rtmp->chunk_size;
      if ((this_output->length % rtmp->chunk_size) > 0) number_of_chunks++;
      //printf("number_of_chunks=%d\n", number_of_chunks);

      unsigned char header[12];
      header[0] = 0x00 | rtmp->chunk_id;

      int_to_byte_array(rtmp->chunk_timestamp + 1, header, 1, 3);
      int_to_byte_array(this_output->length, header, 4, 3);

      header[7] = 0x14;

      header[8] = 0x00;
      header[9] = 0x00;  
      header[10] = 0x00;
      header[11] = 0x00;

      rtmp_allocate_output_message_content(output, 12 + this_output->length + (number_of_chunks - 1));

      int output_index=0, input_index=0;
      memcpy(output->message, header, 12); output_index += 12;

      unsigned char chunk_header = 0b11000011;

      int chunk_index=0;
      for (chunk_index=0;chunk_index<number_of_chunks;chunk_index++) {
        int size = rtmp->chunk_size;
        if ((this_output->length - input_index) < (unsigned int)size)
          size = (this_output->length - input_index);

        if (chunk_index > 0) {
          memcpy(output->message + output_index, &chunk_header, 1); output_index += 1;
        }

        memcpy(output->message + output_index, 
                this_output->message + input_index, 
                size);

        output_index += size;
        input_index += size;
      }
    }

    int i;
    for (i=0;i<output->length;i++) {
      //printf("%x ", output->message[i]);
    }
    //printf("\n");

    rtmp_destroy_output_message(this_output);
    free(rtmp->message);
  }

  return 1;
}

int rtmp_process_read_nothing(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output) {
  (void) rtmp;
  (void) output;

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

void rtmp_process_message(Rtmp *rtmp, RtmpOutputMessage *output) {
  printf("message_type: %x\n", rtmp->message_type);

  if (rtmp->message_type == 0x14) {
    return rtmp_process_command_message(rtmp, output);
  } else if (rtmp->message_type == 0x09) {
    int start = 0;

    // while (start < rtmp->message_length) {
    //   unsigned int payload_length = chars_to_int(rtmp->message + start + 1, 3);

      // printf("====\n");
      // printf("message_type=%u\n", rtmp->message[start]);
      // printf("payload_length=%u\n", payload_length);
      // printf("timestamp=%u\n", chars_to_int(rtmp->message + start + 1 + 3, 4));
      // printf("stream_id=%u\n", chars_to_int(rtmp->message + start + 1 + 3 + 4, 3));
      // printf("====\n");
      // break;
    // }

    printf("t=%u\n", rtmp->chunk_timestamp);

    unsigned char video_tag_header[] = {
      0x09,
      0x00, 0x00, 0x00,
      0x00, 0x00, 0x00,
      0x00,
      0x00, 0x00, 0x00
    };

    unsigned char end_tag[] = {
      0x00, 0x00, 0x00, 0x00
    };

    int_to_byte_array(rtmp->message_length, video_tag_header, 1, 3);
    int_to_byte_array(rtmp->chunk_timestamp, video_tag_header, 4, 3);
    int_to_byte_array(rtmp->message_length + 11, end_tag, 0, 4);

    fwrite(video_tag_header, 1, 11, rtmp->file);
    fwrite(rtmp->message, 1, rtmp->message_length, rtmp->file);
    fwrite(end_tag, 1, 4, rtmp->file);

  } else if (rtmp->message_type == 0x11) {
    printf("AMF3 is not supported.\n");
    exit(1);
  } else if (rtmp->message_type == 0x02) {
    printf("Abort\n");
    if (rtmp->file != NULL) fclose(rtmp->file);
    exit(1);
  } else {
    printf("Unsupported Message Type\n");
    exit(1);
  }
}

struct tagbstring COMMAND_CONNECT = bsStatic("connect");
struct tagbstring COMMAND_CREATE_STREAM = bsStatic("createStream");
struct tagbstring COMMAND_CLOSE_STREAM = bsStatic("closeStream");
struct tagbstring COMMAND_DELETE_STREAM = bsStatic("deleteStream");
struct tagbstring COMMAND_PUBLISH = bsStatic("publish");
void rtmp_process_command_message(Rtmp *rtmp, RtmpOutputMessage *output) {
  bstring command; 
  amf0_deserialize_string(&(command), rtmp->message + 1);

  printf("Command: %s\n", bdata(command));
  
  if (bstrcmp(command, &COMMAND_CONNECT) == 0) {
    rtmp_process_connect_message(rtmp, output);
  } else if (bstrcmp(command, &COMMAND_CREATE_STREAM) == 0) {
    rtmp_process_create_stream_message(rtmp, output);
  } else if (bstrcmp(command, &COMMAND_CLOSE_STREAM) == 0
            || bstrcmp(command, &COMMAND_DELETE_STREAM) == 0) {
    rtmp->end = 1;
    if (rtmp->file != NULL) fclose(rtmp->file);
  } else if (bstrcmp(command, &COMMAND_PUBLISH) == 0) {
    rtmp_process_publish_message(rtmp, output);
  }
  
  bdestroy(command);
}

void rtmp_process_connect_message(Rtmp *rtmp, RtmpOutputMessage *output) {
  Amf0ConnectMessage *cmd = amf0_create_connect_message();
  amf0_deserialize_connect_message(cmd, rtmp->message);

  printf("Transaction ID: %f\n", cmd->transaction_id);
  printf("Arguments:\n");
  print_amf0_object(cmd->arguments);
  printf("\n");
  
  amf0_destroy_connect_message(cmd);

  Amf0ResponseConnectMessage *msg = amf0_create_response_connect_message();

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
  int count = amf0_serialize_response_connect_message(write, msg);
  //printf("count=%d\n", count);

  amf0_destroy_response_connect_message(msg);

  rtmp_allocate_output_message_content(output, count);
  memcpy(output->message, write, count);
}

void rtmp_process_create_stream_message(Rtmp *rtmp, RtmpOutputMessage *output) {
  Amf0CreateStreamMessage *cmd = amf0_create_create_stream_message();
  amf0_deserialize_create_stream_message(cmd, rtmp->message);

  printf("Transaction ID: %f\n", cmd->transaction_id);
  if (cmd->object == NULL) {
    printf("Object: NULL\n");
  } else {
    printf("Object:\n");
    print_amf0_object(cmd->object);
  }
  printf("\n");
  
  amf0_destroy_create_stream_message(cmd);

  Amf0ResponseCreateStreamMessage *response = amf0_create_response_create_stream_message();

  response->command = bfromcstr("_result");
  response->transaction_id = 2.0;
  response->object = NULL;
  response->stream_id = 1.0;

  unsigned char write[65536];
  int count = amf0_serialize_response_create_stream_message(write, response);
  //printf("count=%d\n", count);

  amf0_destroy_response_create_stream_message(response);

  rtmp_allocate_output_message_content(output, count);
  memcpy(output->message, write, count);
}

void rtmp_process_publish_message(Rtmp *rtmp, RtmpOutputMessage *output) {
  Amf0PublishMessage *cmd = amf0_create_publish_message();
  amf0_deserialize_publish_message(cmd, rtmp->message);

  printf("Transaction ID: %f\n", cmd->transaction_id);
  printf("Publishing Name: %s\n", bdata(cmd->publishing_name));
  printf("Publishing Type: %s\n", bdata(cmd->publishing_type));

  amf0_destroy_publish_message(cmd);

  // rtmp_allocate_output_message_content(ack, 27);

  // int i = 0;
  // unsigned char *msg = ack->message;
  // msg[i++] = 0x02;

  // msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x00;
  // int_to_byte_array(6, msg, i, 3);i += 3;

  // msg[i++] = 0x04;

  // msg[i++] = 0x00;
  // msg[i++] = 0x00;  
  // msg[i++] = 0x00;
  // msg[i++] = 0x00;

  // msg[i++] = 0x04;

  // msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x04;
  // msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x00;
  // msg[i++] = 0x00;msg[i++] = 0x00;msg[i++] = 0x00;

  rtmp->file = fopen("raw_data/file.flv", "w");

  unsigned char flv_header[] = {
    0x46, 0x4c, 0x56,
    0x01,
    0x01,
    0x00, 0x00, 0x00, 0x09,
    0x00, 0x00, 0x00, 0x00
  };
  fwrite(flv_header, 1, 13, rtmp->file);
}
