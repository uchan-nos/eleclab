/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * メッセージキュー
 */
#include "main.h"

static volatile QueueElemType queue[QUEUE_CAP];
static uint8_t rp = 0, wp = 0, free = QUEUE_CAP;

#define INC_PTR(p) \
  do {\
    if (++p >= QUEUE_CAP) {\
      p = 0;\
    }\
  } while (0)

uint8_t Queue_IsEmpty(void) {
  return free == QUEUE_CAP;
}

QueueElemType Queue_Pop(void) {
  QueueElemType v = 0;
  if (free < QUEUE_CAP) {
    v = queue[rp];
    INC_PTR(rp);
    ++free;
  }
  return v;
}

void Queue_Push(QueueElemType v) {
  if (free > 0) {
    queue[wp] = v;
    INC_PTR(wp);
    --free;
  }
}
