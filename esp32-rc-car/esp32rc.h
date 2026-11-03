#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_now.h"

#define ESPNOW_WIFI_CHANNEL 0
#define MAX_ESP_NOW_MAC_LEN 6
#define BAUD 115200
#define NO_PMK_KEY false

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

size_t FormatMACAddr(char *s, const uint8_t *mac);
void PrintEspResult(esp_err_t err, const char *act_name);

esp_err_t AddPeer(uint8_t *mac);
void StartWiFi();
void PrintMyMACAddr();
