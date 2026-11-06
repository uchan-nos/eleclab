#include <Arduino.h>
#include "WiFi.h"
#include "esp_now.h"
#include "../esp32rc.h"

// 送信側マイコンのMACアドレスを ../mac_controller.txt にカンマ区切りで書く
// 雛形ファイル：../mac_example.txt
static uint8_t ControllerMACAddr[MAX_ESP_NOW_MAC_LEN] = {
#include "../mac_controller.txt"
};


void RecvCommand_cb(const esp_now_recv_info *info, const uint8_t *incomingData, int len);
void SendResponse();
void SendResponse_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);

void setup() {
  Serial.begin(BAUD);
  //while(!Serial);
  StartWiFi();
  PrintMyMACAddr();
  PrintEspResult(esp_now_init(), "init ESP-NOW");

  esp_now_register_recv_cb(RecvCommand_cb);
  esp_now_register_send_cb(SendResponse_cb);

  AddPeer(ControllerMACAddr);

  pinMode(D2, OUTPUT); // INB
  pinMode(D3, OUTPUT); // INA
  digitalWrite(D2, LOW);
  digitalWrite(D3, LOW);
}

void loop() {
  Serial.println("looping...");
  delay(1000);
}

void RecvCommand_cb(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  RcCommand cmd;
  memcpy(&cmd, incomingData, sizeof(cmd));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("dir: ");
  Serial.println(static_cast<int>(cmd.dir));
  Serial.print("speed: ");
  Serial.println(cmd.speed);
  Serial.println();

  if (cmd.dir == CarDirection::Forward) {
    analogWrite(D2, cmd.speed);
    analogWrite(D3, 0);
    Serial.printf("analogWrite D2=%d, D3=%d\n", (int)cmd.speed, 0);
  } else {
    analogWrite(D2, 0);
    analogWrite(D3, cmd.speed);
    Serial.printf("analogWrite D2=%d, D3=%d\n", 0, (int)cmd.speed);
  }
}
void SendResponse_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status){
  const uint8_t *mac_addr = tx_info->des_addr;
  char macStr[18];
  Serial.print("Packet to: ");
  FormatMACAddr(macStr, mac_addr);
  Serial.println(macStr);
  delay(500);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  Serial.println("");
}

void SendResponse(){
  RcResponse resp{};

  esp_err_t send_res = esp_now_send(ControllerMACAddr, (uint8_t *)&resp, sizeof(resp));

  if (send_res == ESP_OK) {
    Serial.println("Data queued for sending");
  }
}
