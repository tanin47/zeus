#include <rtmp.h>

unsigned int chars_to_int(char *arr, int len) {
  unsigned int ret = 0;
  unsigned int base = 1;

  int i;
  for (i=len-1;i>=0;i--) {
    ret += (unsigned char) arr[i] * base;
    base = base * 256;
  }

  return ret;
}


unsigned int chars_to_int_little_endian(char *arr, int len) {
  unsigned int ret = 0;
  unsigned int base = 1;

  int i;
  for (i=0;i<len;i++) {
    ret += (unsigned char) arr[i] * base;
    base = base * 256;
  }

  return ret;
}

Rtmp *rtmp_create() {
  Rtmp *rtmp = malloc(sizeof(Rtmp));
  rtmp->state = WAIT_FOR_C0;
  rtmp->c1 = NULL;
  return rtmp;
}

void rtmp_destroy(Rtmp *rtmp) {
  if (rtmp->c1 != NULL) free(rtmp->c1);
  free(rtmp);
}

int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  while (RingBuffer_available_data(buffer) > 0) {
    printf("Available data: %d\n", RingBuffer_available_data(buffer));

    int retVal = -1;

    if (rtmp->state == WAIT_FOR_C0) {
      retVal = rtmp_process_c0(rtmp, buffer, write_buffer);
    } else if (rtmp->state == WAIT_FOR_C1) {
      retVal = rtmp_process_c1(rtmp, buffer, write_buffer);
    } else if (rtmp->state == WAIT_FOR_C2) {
      retVal = rtmp_process_c2(rtmp, buffer, write_buffer);
    } else if (rtmp->state == READ_CHUNK_TYPE) {
      retVal = rtmp_process_read_chunk_type(rtmp, buffer, write_buffer);
    } else if (rtmp->state == READ_CHUNK_ID) {
      retVal = rtmp_process_read_chunk_id(rtmp, buffer, write_buffer);
    } else if (rtmp->state == READ_CHUNK_DATA) {
      retVal = rtmp_process_read_chunk_data(rtmp, buffer, write_buffer);
    } else {
      return -1;
    }

    if (retVal == 0) break;
  }

  return 1;
}

int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 1) return 0;

  printf("Process C0, Write S0,S1\n");

  char write[1537];
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

  char write[1536];

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

  char read[1536];
  RingBuffer_read(buffer, read, 1536);

  rtmp->state = READ_CHUNK_TYPE;

  return 1;
}

int rtmp_process_read_chunk_type(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 1) return 0;

  printf("Read chunk type\n");

  RingBuffer_read(buffer, &rtmp->chunk_first_byte, 1);

  unsigned char first = rtmp->chunk_first_byte & 0b10000000;
  unsigned char second = rtmp->chunk_first_byte & 0b01000000;

  if (first == 0 && second == 0) {
    printf("Chunk Type 0\n");
    rtmp->chunk_type = TYPE_0;
  } else if (first == 0 && second > 0) {
    printf("Chunk Type 1\n");
    rtmp->chunk_type = TYPE_1;
  } else if (first > 0 && second == 0) {
    printf("Chunk Type 2\n");
    rtmp->chunk_type = TYPE_2;
  } else {
    printf("Unrecognized chunk type %u %u\n", first, second);
    rtmp->chunk_type = UNRECOGNIZED_CHUNK_TYPE;
  }

  rtmp->state = READ_CHUNK_ID;

  return 1;
}

int rtmp_process_read_chunk_id(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (rtmp->chunk_type == TYPE_0) return rtmp_process_read_chunk_id_type_0(rtmp, buffer, write_buffer);
  else if (rtmp->chunk_type == TYPE_1) return rtmp_process_read_chunk_id_type_1(rtmp, buffer, write_buffer);
  else if (rtmp->chunk_type == TYPE_2) return rtmp_process_read_chunk_id_type_2(rtmp, buffer, write_buffer);
  else printf("Unrecognized chunk_type %d\n", rtmp->chunk_type);

  return -1;
}

int rtmp_process_read_chunk_id_type_0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 1) return 0;

  char byte;
  RingBuffer_read(buffer, &byte, 1);
  rtmp->chunk_id = (unsigned char)byte + 64;

  printf("Chunk ID: %u\n", rtmp->chunk_id);

  rtmp->state = READ_CHUNK_DATA;

  return 1;
}

int rtmp_process_read_chunk_id_type_1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 2) return 0;

  char bytes[2];
  RingBuffer_read(buffer, bytes, 1);
  rtmp->chunk_id = ((unsigned char)bytes[1] * 256) + (unsigned char)bytes[0] + 64;

  printf("Chunk ID: %u\n", rtmp->chunk_id);

  rtmp->state = READ_CHUNK_DATA;

  return 1;
}

int rtmp_process_read_chunk_id_type_2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  return 0;
}


int rtmp_process_read_chunk_data(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (rtmp->chunk_type == TYPE_0) return rtmp_process_read_chunk_data_type_0(rtmp, buffer, write_buffer);
  else if (rtmp->chunk_type == TYPE_1) return rtmp_process_read_chunk_data_type_1(rtmp, buffer, write_buffer);
  else if (rtmp->chunk_type == TYPE_2) return rtmp_process_read_chunk_data_type_2(rtmp, buffer, write_buffer);
  else printf("Unrecognized chunk_type %d\n", rtmp->chunk_type);

  return -1;
}

int rtmp_process_read_chunk_data_type_0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 11) return 0;

  char timestamp[3];
  char msg_length[3];
  char msg_type[1];
  char msg_stream_id[4];

  RingBuffer_read(buffer, timestamp, 3);
  RingBuffer_read(buffer, msg_length, 3);
  RingBuffer_read(buffer, msg_type, 1);
  RingBuffer_read(buffer, msg_stream_id, 4);

  printf("Timestamp=%u, msg_length=%u, msg_type=%u, msg_stream_id=%u\n", 
          chars_to_int(timestamp, 3),
          chars_to_int(msg_length, 3),
          chars_to_int(msg_type, 1),
          chars_to_int_little_endian(msg_stream_id, 4));

  rtmp->state = READ_CHUNK_TYPE;

  return 1;
}

int rtmp_process_read_chunk_data_type_1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  if (RingBuffer_available_data(buffer) < 7) return 0;

  char timestamp[3];
  char msg_length[3];
  char msg_type[1];

  RingBuffer_read(buffer, timestamp, 3);
  RingBuffer_read(buffer, msg_length, 3);
  RingBuffer_read(buffer, msg_type, 1);

  printf("Timestamp=%u, msg_length=%u, msg_type=%u\n", 
          chars_to_int(timestamp, 3),
          chars_to_int(msg_length, 3),
          chars_to_int(msg_type, 1));

  rtmp->state = READ_CHUNK_TYPE;

  return 1;
}

int rtmp_process_read_chunk_data_type_2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer) {
  return 0;
}
