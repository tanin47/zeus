#ifndef __rtmp_h__
#define __rtmp_h__

#include <lcthw/ringbuffer.h>

typedef enum { 
  WAIT_FOR_C0,
  WAIT_FOR_C1,
  WAIT_FOR_C2,
  CONNECTED
} RtmpState;

typedef struct {
  RtmpState state;
} Rtmp;

Rtmp *rtmp_create();
void rtmp_destroy(Rtmp *rtmp);

int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer);
int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer);
int rtmp_process_c1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer);
int rtmp_process_c2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer);
int rtmp_process_connected(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer);

#endif
