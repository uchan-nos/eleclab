#include <stdint.h>

enum class CarDirection : uint8_t {
  Forward,
  Backward,
  Brake,
};

struct RcCommand {
  CarDirection dir;
  uint8_t speed; // 0–255
};

struct RcResponse {
};
