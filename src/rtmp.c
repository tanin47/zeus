#include <rtmp.h>

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

int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  while (RingBuffer_available_data(buffer) > 0) {
    printf("----------------------\n");
    printf("Available data: %d, State=%d\n", RingBuffer_available_data(buffer), rtmp->state);

    int retVal = -1;

    if (rtmp->state == WAIT_FOR_C0) {
      retVal = rtmp_process_c0(rtmp, buffer, write_buffer);
    } else if (rtmp->state == WAIT_FOR_C1) {
      retVal = rtmp_process_c1(rtmp, buffer, write_buffer);
    } else if (rtmp->state == WAIT_FOR_C2) {
      retVal = rtmp_process_c2(rtmp, buffer, write_buffer);
    } else if (rtmp->state == READ_CHUNK_TYPE) {
      retVal = rtmp_process_read_chunk_type(rtmp, buffer, write_buffer);
    } else if (rtmp->state == READ_CHUNK_EXTENDED_ID) {
      retVal = rtmp_process_read_chunk_extended_id(rtmp, buffer, write_buffer);
    } else if (rtmp->state == READ_CHUNK_HEADER) {
      retVal = rtmp_process_read_chunk_header(rtmp, buffer, write_buffer);
    } else if (rtmp->state == READ_CHUNK_EXTENDED_TIMESTAMP) {
      retVal = rtmp_process_read_chunk_extended_timestamp(rtmp, buffer, write_buffer);
    } else if (rtmp->state == READ_CHUNK_DATA) {
      retVal = rtmp_process_read_chunk_data(rtmp, buffer, write_buffer);
    } else {
      return -1;
    }

    printf("Data left: %d, State=%d\n", RingBuffer_available_data(buffer), rtmp->state);

    if (retVal == 0) break;
  }

  return 1;
}

int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 1) return 0;

  printf("Process C0, Write S0,S1\n");

  unsigned char write[1537];
  RingBuffer_read(buffer, write, 1);

  int i;
  for (i=1;i<1537;i++) {
    write[i] = 0;
  }

  RingBuffer_write(write_buffer, write, 1537);

  rtmp->state = WAIT_FOR_C1;

  return 1;
}

int rtmp_process_c1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 1536) return 0;

  printf("Process C1, Write S2\n");

  rtmp->c1 = malloc(sizeof(char) * 1536);
  RingBuffer_read(buffer, rtmp->c1, 1536);

  unsigned char write[1536];

  int i;
  for (i=0;i<1536;i++) {
    write[i] = rtmp->c1[i];
  }

  write[4] = 0;
  write[5] = 0;
  write[6] = 0;
  write[7] = 0;

  RingBuffer_write(write_buffer, write, 1536);

  rtmp->state = WAIT_FOR_C2;

  free(rtmp->c1);
  rtmp->c1 = NULL;

  return 1;
}

int rtmp_process_c2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 1536) return 0;

  printf("Process C2\n");

  unsigned char read[1536];
  RingBuffer_read(buffer, read, 1536);

  rtmp->state = READ_CHUNK_TYPE;

  return 1;
}

int rtmp_process_read_chunk_type(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 1) return 0;

  printf("Read chunk type\n");

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

int rtmp_process_read_chunk_extended_id(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
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
  }

  printf("Chunk ID: %u\n", rtmp->chunk_id);
  rtmp->state = READ_CHUNK_HEADER;

  return 1;
}

int rtmp_process_read_chunk_header(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (rtmp->chunk_type == TYPE_0) return rtmp_process_read_chunk_header_type_0(rtmp, buffer, write_buffer);
  else if (rtmp->chunk_type == TYPE_1) return rtmp_process_read_chunk_header_type_1(rtmp, buffer, write_buffer);
  else if (rtmp->chunk_type == TYPE_2) return rtmp_process_read_chunk_header_type_2(rtmp, buffer, write_buffer);
  else if (rtmp->chunk_type == TYPE_3) return rtmp_process_read_chunk_header_type_3(rtmp, buffer, write_buffer);
  else printf("Unrecognized chunk_type %d\n", rtmp->chunk_type);

  return -1;
}

int rtmp_process_read_chunk_header_type_0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 11) return 0;

  unsigned char timestamp[3];
  unsigned char packet_length[3];
  unsigned char msg_type[1];
  unsigned char msg_stream_id[4];

  RingBuffer_read(buffer, timestamp, 3);
  RingBuffer_read(buffer, packet_length, 3);
  RingBuffer_read(buffer, msg_type, 1);
  RingBuffer_read(buffer, msg_stream_id, 4);

  rtmp->chunk_timestamp = chars_to_int(timestamp, 3);
  rtmp->packet_length = chars_to_int(packet_length, 3);

  printf("Read header type 0\n");
  printf("Timestamp=%u, packet_length=%u, msg_type=%u, msg_stream_id=%u\n", 
          rtmp->chunk_timestamp,
          rtmp->packet_length,
          chars_to_int(msg_type, 1),
          chars_to_int_little_endian(msg_stream_id, 4));

  if (rtmp->chunk_timestamp == 0xffffff) {
    rtmp->state = READ_CHUNK_EXTENDED_TIMESTAMP;
  } else {    
    rtmp->message = malloc(sizeof(unsigned char) * rtmp->packet_length);
    rtmp->chunk_write_to = 0;
    rtmp->state = READ_CHUNK_DATA;
  }

  return 1;
}

int rtmp_process_read_chunk_header_type_1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 7) return 0;

  unsigned char timestamp[3];
  unsigned char packet_length[3];
  unsigned char msg_type[1];

  RingBuffer_read(buffer, timestamp, 3);
  RingBuffer_read(buffer, packet_length, 3);
  RingBuffer_read(buffer, msg_type, 1);

  rtmp->chunk_timestamp = chars_to_int(timestamp, 3);
  rtmp->packet_length = chars_to_int(packet_length, 3);

  printf("Read header type 1\n");
  printf("Timestamp=%u, packet_length=%u, msg_type=%u\n", 
          rtmp->chunk_timestamp,
          rtmp->packet_length,
          chars_to_int(msg_type, 1));

  if (rtmp->chunk_timestamp == 0xffffff) {
    rtmp->state = READ_CHUNK_EXTENDED_TIMESTAMP;
  } else {    
    rtmp->message = malloc(sizeof(char) * rtmp->packet_length);
    rtmp->chunk_write_to = 0;
    rtmp->state = READ_CHUNK_DATA;
  }
  
  return 1;
}

int rtmp_process_read_chunk_header_type_2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 3) return 0;

  unsigned char timestamp[3];
  RingBuffer_read(buffer, timestamp, 3);
  rtmp->chunk_timestamp = chars_to_int(timestamp, 3);

  printf("Read header type 2\n");
  printf("Timestamp=%u\n", 
          rtmp->chunk_timestamp);

  if (rtmp->chunk_timestamp == 0xffffff) {
    rtmp->state = READ_CHUNK_EXTENDED_TIMESTAMP;
  } else {    
    rtmp->message = malloc(sizeof(char) * rtmp->packet_length);
    rtmp->chunk_write_to = 0;
    rtmp->state = READ_CHUNK_DATA;
  }

  return 1;
}

int rtmp_process_read_chunk_header_type_3(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) { 
  printf("Read header type 3\n");
  rtmp->state = READ_CHUNK_DATA;
  return 1;
}


int rtmp_process_read_chunk_extended_timestamp(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 4) return 0;

  printf("Read extended timestamp.\n");

  char extended_timestamp[4];
  RingBuffer_read(buffer, extended_timestamp, 4);
  
  rtmp->state = READ_CHUNK_DATA;

  return 1;
}


int rtmp_process_read_chunk_data(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  // if (rtmp->chunk_type == TYPE_0) return rtmp_process_read_chunk_data_type_0(rtmp, buffer, write_buffer);
  // else if (rtmp->chunk_type == TYPE_1) return rtmp_process_read_chunk_data_type_1(rtmp, buffer, write_buffer);
  // else if (rtmp->chunk_type == TYPE_2) return rtmp_process_read_chunk_data_type_2(rtmp, buffer, write_buffer);
  // else if (rtmp->chunk_type == TYPE_3) return rtmp_process_read_chunk_data_type_3(rtmp, buffer, write_buffer);
  // else printf("Unrecognized chunk_type %d\n", rtmp->chunk_type);

  // return -1;

  return rtmp_process_read_chunk_data_type_0(rtmp, buffer, write_buffer);
}

int rtmp_process_read_chunk_data_type_0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  int read_amount = rtmp->packet_length;
  if (rtmp->chunk_size < read_amount) 
    read_amount = rtmp->chunk_size;

  if (RingBuffer_available_data(buffer) < read_amount) return 0;

  printf("Read data: %d, write_start: %d\n",rtmp->message, rtmp->chunk_write_to);

  RingBuffer_read(buffer, rtmp->message + rtmp->chunk_write_to, read_amount);
  rtmp->chunk_write_to += read_amount;
  rtmp->packet_length -= read_amount;

  printf("Finish read data: %d\n", read_amount);

  rtmp->state = READ_CHUNK_TYPE;

  if (rtmp->packet_length == 0) {
    int i;
    for (i=0;i<rtmp->chunk_write_to;i++) {
      printf("%X ", rtmp->message[i]);
    }

    printf("\n");

    amf0_parse(rtmp->message, 0, rtmp->chunk_write_to);
    free(rtmp->message);
    rtmp->message = NULL;
  }

  return 1;
}

int rtmp_process_read_chunk_data_type_1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  return 0;
}

int rtmp_process_read_chunk_data_type_2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  return 0;
}

int rtmp_process_read_chunk_data_type_3(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  return 0;
}