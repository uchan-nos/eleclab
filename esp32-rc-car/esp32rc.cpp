#include "esp32rc.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// s は 12 + 5 + 1 = 18 バイト以上必要
size_t FormatMACAddr(char *s, const uint8_t *mac) {
  return snprintf(s, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void PrintEspResult(esp_err_t err, const char *act_name) {
  if (err == ESP_OK) {
    Serial.print("Successful to");
    Serial.print(act_name);
    Serial.println();
  } else {
    Serial.print("Failed to");
    Serial.print(act_name);
    Serial.println(err);
  }
}

esp_err_t AddPeer(uint8_t *mac) {
  char s[18];
  FormatMACAddr(s, mac);
  Serial.printf("peer MAC addr = %s", s);
  Serial.println();

  esp_now_peer_info_t peer_info;
  peer_info.channel = ESPNOW_WIFI_CHANNEL;
  peer_info.encrypt = NO_PMK_KEY;
  memcpy(peer_info.peer_addr, mac, 6);

  esp_err_t add_peer_res = esp_now_add_peer(&peer_info);
  PrintEspResult(add_peer_res, "add peer");
  return add_peer_res;
}

void StartWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while(!WiFi.STA.started()){
    Serial.print(".");
    delay(100);
  }
}

void PrintMyMACAddr() {
  uint8_t mac[MAX_ESP_NOW_MAC_LEN];
  WiFi.macAddress(mac);
  Serial.println();
  Serial.printf("const uint8_t mac_self[6] = {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x};", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println();
}
