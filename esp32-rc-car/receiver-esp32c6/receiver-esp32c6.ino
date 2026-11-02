#include<Arduino.h>
#include "WiFi.h"
#include "esp_now.h"
#include "../esp32rc.h"

#define ESPNOW_WIFI_CHANNEL 0
#define MAX_ESP_NOW_MAC_LEN 6
#define BAUD 115200
#define MAX_CHARACTERS_NUMBER 20
#define NO_PMK_KEY false

typedef uint8_t XIAO;
typedef int status;

// 送信側マイコンのMACアドレスを ../mac_controller.txt にカンマ区切りで書く
// 雛形ファイル：../mac_example.txt
static uint8_t ControllerMACAddr[MAX_ESP_NOW_MAC_LEN] = {
#include "../mac_controller.txt"
};

esp_now_peer_info_t peerInfo_sender;

void Receiver_MACAddress_requir();
void espnow_init();
void espnow_deinit();
void ReceiverXIAOC6_Recive_Data_cb(const uint8_t * mac, const uint8_t *incomingData, int len);
void SendResponse();
void SendResponse_cb(const XIAO *mac_addr,esp_now_send_status_t status);
void AssociateController();

void setup() {
  Serial.begin(BAUD);
  while(!Serial);
  Receiver_MACAddress_requir();
  espnow_init();

  esp_now_register_recv_cb(ReceiverXIAOC6_Recive_Data_cb);

  esp_now_register_send_cb(SendResponse_cb);
  AssociateController();  
}

void loop() {
  Serial.println("looping...");
  delay(1000);
}

void espnow_init(){
  status espnow_sign = esp_now_init();
  if(espnow_sign == ESP_OK)
  {
    Serial.println("the esp now is successful init!");
  }else
  {
    Serial.println("the esp now is failed init");
  }
}

void espnow_deinit(){
  status espnow_sign = esp_now_deinit();
  if(espnow_sign == ESP_OK){
    Serial.println("the esp now is successful deinit!");
  }else
  {
    Serial.println("the esp now is failed deinit!");
  }
}

void Receiver_MACAddress_requir(){
    WiFi.mode(WIFI_STA);
    WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
    XIAO mac[MAX_ESP_NOW_MAC_LEN];
    while(!WiFi.STA.started()){
      Serial.print(".");
      delay(100);
    }
      WiFi.macAddress(mac);
      Serial.println();
      Serial.printf("const uint8_t mac_self[6] = {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x};", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      Serial.println();
}

void ReceiverXIAOC6_Recive_Data_cb(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  RcCommand cmd;
  memcpy(&cmd, incomingData, sizeof(cmd));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("dir: ");
  Serial.println(static_cast<int>(cmd.dir));
  Serial.print("speed: ");
  Serial.println(cmd.speed);
  Serial.println();
}
void SendResponse_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status){
  const uint8_t *mac_addr = tx_info->des_addr;
  char macStr[18];
  Serial.print("Packet to: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
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

void AssociateController(){
  Serial.println("Attempting to associate controller...");
  peerInfo_sender.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo_sender.encrypt = NO_PMK_KEY;

  Serial.printf("controller MAC addr = %02x:%02x:%02x:%02x:%02x:%02x",
    ControllerMACAddr[0], ControllerMACAddr[1], ControllerMACAddr[2],
    ControllerMACAddr[3], ControllerMACAddr[4], ControllerMACAddr[5]);
  Serial.println();

  memcpy(peerInfo_sender.peer_addr, ControllerMACAddr, 6);
  esp_err_t add_peer_res = esp_now_add_peer(&peerInfo_sender);
  if (add_peer_res != ESP_OK) {
    Serial.print("Failed to add peer");
    Serial.println(add_peer_res);
  } else {
    Serial.println("Successful to add peer");
  }
}