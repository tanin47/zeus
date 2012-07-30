#ifndef __rtmp_h__
#define __rtmp_h__

#include <lcthw/ringbuffer.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum { 
  UNRECOGNIZED_STATE,
  WAIT_FOR_C0,
  WAIT_FOR_C1,
  WAIT_FOR_C2,
  READ_CHUNK_TYPE,
  READ_CHUNK_ID,
  READ_CHUNK_DATA
} RtmpState;

typedef enum {
  UNRECOGNIZED_CHUNK_TYPE, 
  TYPE_0,
  TYPE_1,
  TYPE_2
} RtmpChunkType;

typedef struct {
  RtmpState state;
  char *c1;


  RtmpChunkType chunk_type;
  char chunk_first_byte;
  unsigned int chunk_id;

} Rtmp;

unsigned int chars_to_int(char *arr, int len);
unsigned int chars_to_int_little_endian(char *arr, int len);

Rtmp *rtmp_create();
void rtmp_destroy(Rtmp *rtmp);

int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_c1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_c2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);

int rtmp_process_read_chunk_type(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);

int rtmp_process_read_chunk_id(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_id_type_0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_id_type_1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_id_type_2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);

int rtmp_process_read_chunk_data(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_data_type_0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_data_type_1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);
int rtmp_process_read_chunk_data_type_2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *write_buffer);


#endif
