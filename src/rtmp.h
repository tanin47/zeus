#ifndef __rtmp_h__
#define __rtmp_h__

#include <lcthw/ringbuffer.h>
#include <stdlib.h>
#include <stdio.h>
#include <basic.h>
#include <amf0.h>

typedef enum { 
  UNRECOGNIZED_STATE,
  WAIT_FOR_C0,
  WAIT_FOR_C1,
  WAIT_FOR_C2,
  READ_CHUNK_TYPE,
  READ_CHUNK_EXTENDED_ID,
  READ_CHUNK_HEADER,
  READ_CHUNK_EXTENDED_TIMESTAMP,
  READ_CHUNK_DATA
} RtmpState;

typedef enum {
  UNRECOGNIZED_CHUNK_TYPE, 
  TYPE_0,
  TYPE_1,
  TYPE_2,
  TYPE_3
} RtmpChunkType;

typedef struct {
  RtmpState state;
  char *c1;


  RtmpChunkType chunk_type;
  unsigned int chunk_id;
  unsigned int chunk_timestamp;
  unsigned int packet_length;
  unsigned int chunk_size;
  int chunk_write_to;

  unsigned char *message;
} Rtmp;

Rtmp *rtmp_create();
void rtmp_destroy(Rtmp *rtmp);

int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_c1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_c2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);

int rtmp_process_read_chunk_type(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);

int rtmp_process_read_chunk_extended_id(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);

int rtmp_process_read_chunk_header(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_header_type_0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_header_type_1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_header_type_2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_header_type_3(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);

int rtmp_process_read_chunk_extended_timestamp(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);

int rtmp_process_read_chunk_data(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_data_type_0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_data_type_1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_data_type_2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_data_type_3(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);


#endif
