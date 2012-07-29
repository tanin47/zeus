#include <rtmp.h>

Rtmp *rtmp_create() {
  Rtmp *rtmp = malloc(sizeof(Rtmp));
  return rtmp;
}

void rtmp_destroy(Rtmp *rtmp) {
  free(rtmp);
}

int rtmp_multiplex(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer) {
  printf("Available data: %d\n", RingBuffer_available_data(buffer));

  char buffer_test[1000];
  printf("Read: %d\n", RingBuffer_read(buffer, buffer_test, RingBuffer_available_data(buffer)-1));
  printf("Got %s", buffer_test);

  RingBuffer_write(writeBuffer, "Hello\n\0", 7);
  return 1;

  if (rtmp->state == WAIT_FOR_C0) {
    return rtmp_process_c0(rtmp, buffer, writeBuffer);
  } else if (rtmp->state == WAIT_FOR_C1) {
    return rtmp_process_c1(rtmp, buffer, writeBuffer);
  } else if (rtmp->state == WAIT_FOR_C2) {
    return rtmp_process_c1(rtmp, buffer, writeBuffer);
  } else if (rtmp->state == CONNECTED) {
    return rtmp_process_connected(rtmp, buffer, writeBuffer);
  } else {
    return -1;
  }
}

int rtmp_process_c0(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer) {
  return 0;
}

int rtmp_process_c1(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer) {
  return 0;
}

int rtmp_process_c2(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer) {
  return 0;
}

int rtmp_process_connected(Rtmp *rtmp, RingBuffer *buffer, RingBuffer *writeBuffer) {
  return 0;
}