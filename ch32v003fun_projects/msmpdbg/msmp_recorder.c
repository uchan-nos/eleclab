#include "ch32fun.h"

#include <stdbool.h>
#include <stdint.h>
#include "msmpdbg.h"

volatile tick_t sig_buf[SIG_BUF_LEN];
volatile size_t sig_wpos;
volatile bool sig_record_mode;
volatile uint32_t sig_record_period_ticks = 2 * SIG_RECORD_RATE;

bool SenseSignal(tick_t tick, bool sig) {
  // メッセージ先頭バイトのスタートビットを検出
  const bool msg_start = msmp_state == MSTATE_IDLE && !sig;
  if (msg_start) {
    msmp_state = MSTATE_ADDR;
  }
  if (sig_record_mode && sig_wpos < SIG_BUF_LEN) {
    // sig_wpos == 0: sig == 0
    // sig_wpos == 1: sig == 1
    // sig_wpos == 2: sig == 0
    // つまり sig_wpos が偶数 => sig == 0

    if (sig_wpos == 0 && sig == 0) {
      // スタートビットを受信した
      sig_buf[sig_wpos++] = tick;
    } else if (sig_buf[0] + sig_record_period_ticks <= tick) {
      // 記録開始後ある程度時間が経過したので記録を終わる
      sig_record_mode = false;
    } else if (sig_wpos > 0 && sig == (sig_wpos & 1)) {
      // 前回の信号と切り替わった
      sig_buf[sig_wpos++] = tick;
    }
  }

  return msg_start;
}

void PlotSignal(int tick_step) {
  size_t sig_len = sig_wpos;
  if (sig_len == 0) {
    return;
  }
  putchar('~');
  size_t sig_i = 0;
  tick_t t = sig_buf[0];
  tick_t end_tick = sig_buf[sig_len - 1];
  bool sig = 0;
  for (tick_t t = sig_buf[0]; t < end_tick; t += tick_step) {
    // 1 ステップ内の信号変化を数える
    size_t sig_change_count = 0;
    for (size_t j = sig_i + 1; j < sig_len && sig_buf[j] < t + tick_step; ++j) {
      ++sig_change_count;
      ++sig_i;
    }

    if (sig_change_count == 0) {
      // 1 ステップの間に信号の変化が無い
      putchar(sig ? '~' : '_');
    } else if (sig_change_count == 1) {
      // 1 ステップの間に 1 回信号が変化
      const bool change_in_early = (sig_buf[sig_i] - t) < (tick_step / 4);
      if (change_in_early) {
        // 1 ステップの前半 1/4 で信号が変化
        putchar(sig ? '_' : '~');
      } else {
        // 1 ステップの途中で信号が変化
        putchar(sig ? '\\' : '/');
      }
    } else {
      // 1 ステップの間に 2 回以上信号が変化
      putchar('X');
    }
    sig ^= sig_change_count & 1;
  }
}

void ProcByte(uint8_t c) {
  switch (msmp_state) {
  case MSTATE_IDLE:
    // ここには来ないはず。IDLE のときにスタートビットを検知した直後に ADDR に進むはずだから。
    printf("Error: must not happen\r\n");
    while (1);
    break;
  case MSTATE_ADDR:
    msg_body_wpos = 0;
    msg_buf[msg_wpos].addr = c;
    msmp_state = MSTATE_LEN;
    break;
  case MSTATE_LEN:
    msg_buf[msg_wpos].len = c;
    msmp_state = MSTATE_BODY;
    break;
  case MSTATE_BODY:
    msg_buf[msg_wpos].body[msg_body_wpos++] = c;
    if (msg_body_wpos == msg_buf[msg_wpos].len) {
      // メッセージ受信完了
      msmp_state = MSTATE_IDLE;
      ++msg_wpos;
      if (msg_wpos >= MSG_BUF_LEN) {
        msg_wpos = 0;
      }
    }
    break;
  }
}

void DumpMessages(size_t msg_num) {
  printf("state=%s\r\n",
         msmp_state == MSTATE_IDLE ? "IDLE: Waiting addr byte" :
         msmp_state == MSTATE_ADDR ? "ADDR: Receiving addr byte" :
         msmp_state == MSTATE_LEN ?  "LEN: Receiving len byte" :
         msmp_state == MSTATE_BODY ? "BODY: Receiving body" : "Unknown");
  size_t last_msg_i = msg_wpos;
  if (msmp_state == MSTATE_IDLE) {
    last_msg_i = (last_msg_i - 1) % MSG_BUF_LEN;
  }
  for (size_t i = 0; i < msg_num; ++i) {
    size_t msg_i = last_msg_i - i;
    if (msg_i >= MSG_BUF_LEN) {
      // 0 を超えたらオーバーフローする。そしたら補正。
      msg_i += MSG_BUF_LEN;
    }
    printf("[%d] addr: %02x, len: %02x, body: ",
           i, msg_buf[msg_i].addr, msg_buf[msg_i].len);
    for (size_t j = 0; j < msg_buf[msg_i].len; ++j) {
      putchar(msg_buf[msg_i].body[j]);
    }
    putchar('\r');
    putchar('\n');
  }
}
