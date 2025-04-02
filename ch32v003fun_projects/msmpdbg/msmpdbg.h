#include <stdint.h>

typedef uint32_t tick_t;

struct Message {
    tick_t start_tick; // スタートビットを受信したときの時刻
    union {
        struct {
            uint8_t addr;
            uint8_t len;
            uint8_t body[63];
        };
        uint8_t raw_msg[65];
    } __attribute__((packed));
};