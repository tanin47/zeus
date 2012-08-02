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
  READ_CHUNK_DATA,
  READ_NOTHING
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
  unsigned int chunk_size;

  RtmpChunkType chunk_type;
  unsigned int chunk_id;
  unsigned int chunk_timestamp;
  unsigned int message_type;
  unsigned int message_stream_id;

  unsigned char *message;
  unsigned int message_length;
  unsigned int message_data_left;
} Rtmp;

typedef struct {
  unsigned int length;
  unsigned int data_left;
  unsigned char *message;
} RtmpOutputMessage;


Rtmp *rtmp_create_output_message(RtmpOutputMessage *output);
void rtmp_allocate_output_message_content(RtmpOutputMessage *output, unsigned int length);
void rtmp_destroy_output_message(RtmpOutputMessage *output);
unsigned int rtmp_output_message_start_at(RtmpOutputMessage *output);

Rtmp *rtmp_create();
void rtmp_destroy(Rtmp *rtmp);


// RTMP Chunking Layer
int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_c1(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_c2(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);

int rtmp_process_read_chunk_type(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);

int rtmp_process_read_chunk_extended_id(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);

int rtmp_process_read_chunk_header(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_read_chunk_header_type_0(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_read_chunk_header_type_1(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_read_chunk_header_type_2(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_read_chunk_header_type_3(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);

int rtmp_read_extended_timestamp_or_data(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_read_chunk_extended_timestamp(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);
int rtmp_process_read_chunk_data(Rtmp *rtmp, RingBuffer *buffer, RtmpOutputMessage *output);

// RTMP Application Layer
int rtmp_process_process_message();


#endif
