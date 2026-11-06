#include <Arduino.h>
#include "WiFi.h"
#include "esp_now.h"
#include "../esp32rc.h"

// 受信側マイコンのMACアドレスを ../mac_receiver.txt にカンマ区切りで書く
// 雛形ファイル：../mac_example.txt
static uint8_t ReceiverMACAddr[MAX_ESP_NOW_MAC_LEN] = {
#include "../mac_receiver.txt"
};

void SendCommand();
void SendCommand_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);
void RecvResponse_cb(const esp_now_recv_info *info, const uint8_t *incomingData, int len);

int joystick_y_avg;

void setup(){
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < 10; ++i) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
  Serial.begin(BAUD);
  StartWiFi();
  PrintMyMACAddr();
  PrintEspResult(esp_now_init(), "init ESP-NOW");

  esp_now_register_recv_cb(RecvResponse_cb);
  esp_now_register_send_cb(SendCommand_cb);

  AddPeer(ReceiverMACAddr);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT); // X
  pinMode(A2, INPUT); // Y


  joystick_y_avg = 0;
  for (int i = 0; i < 16; ++i) {
    joystick_y_avg += analogReadMilliVolts(A2);
  }
  joystick_y_avg >>= 4;
}

void loop(){
  SendCommand();
  delay(100);
}

void SendCommand_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status){
  return;

  const uint8_t *mac_addr = tx_info->des_addr;
  char macStr[18];
  Serial.print("Packet to: ");
  FormatMACAddr(macStr, mac_addr);
  Serial.println(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  Serial.println("");
}

static uint8_t speed = 10;
void SendCommand(){
  RcCommand cmd{
    CarDirection::Forward,
    0,
  };

  int y = analogReadMilliVolts(A2);
  if (y < joystick_y_avg) {
    cmd.dir = CarDirection::Backward;
    y = joystick_y_avg - y;
  } else {
    y -= joystick_y_avg;
  }

  y = (y * 255) / (3300/2);
  if (y > 255) {
    y = 255;
  }

  cmd.speed = y;
  Serial.printf("dir=%d speed=%3d y=%4d avg=%4d\n", (int)cmd.dir, (int)cmd.speed, y, joystick_y_avg);

  esp_err_t send_res = esp_now_send(ReceiverMACAddr, (uint8_t *)&cmd, sizeof(cmd));

  if (send_res == ESP_OK) {
    Serial.println("Data queued for sending");
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    for (int i = 0; i < 10; ++i) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
  }
}

void RecvResponse_cb(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  RcResponse resp;
  memcpy(&resp, incomingData, sizeof(resp));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.println();
}
