#include<Arduino.h>
#include "WiFi.h"
#include "esp_now.h"

#define ESPNOW_WIFI_CHANNEL 0
#define MAX_ESP_NOW_MAC_LEN 6
#define BAUD 115200
#define MAX_CHARACTERS_NUMBER 20
#define NO_PMK_KEY false

typedef uint8_t XIAO;
typedef int status;

//You need input your XIAO ESP32 Series MAC,Cannot copy directly!!!!
static uint8_t XIAOS3_Sender_MAC_Address[MAX_ESP_NOW_MAC_LEN] = {0xcc, 0x8d, 0xa2, 0x0c, 0x57, 0x5c};

esp_now_peer_info_t peerInfo_sender;

typedef struct receiver_meesage_types{
  char Reveiver_device[MAX_CHARACTERS_NUMBER];
  char Reveiver_Trag[MAX_CHARACTERS_NUMBER];
}receiver_meesage_types;

receiver_meesage_types XIAOC6_RECEIVER_INFORATION;

typedef struct message_types{
  char Sender_device[MAX_CHARACTERS_NUMBER];
  char Sender_Trag[MAX_CHARACTERS_NUMBER];
}message_types;

message_types XIAOS3_SENDER_INFORATION;

void Receiver_MACAddress_requir();
void espnow_init();
void espnow_deinit();
void ReceiverXIAOC6_Recive_Data_cb(const uint8_t * mac, const uint8_t *incomingData, int len);
void ReceiverXIAOC6_Send_Data();
void ReceiverXIAOC6_Send_Data_cb(const XIAO *mac_addr,esp_now_send_status_t status);
void Association_SenderXIAOS3_peer();

void setup() {
  Serial.begin(BAUD);
  while(!Serial);
  Receiver_MACAddress_requir();
  espnow_init();

  esp_now_register_recv_cb(ReceiverXIAOC6_Recive_Data_cb);

  esp_now_register_send_cb(ReceiverXIAOC6_Send_Data_cb);
  Association_SenderXIAOS3_peer();  
}

void loop() {
  ReceiverXIAOC6_Send_Data();
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
  memcpy(&XIAOS3_SENDER_INFORATION, incomingData, sizeof(XIAOS3_SENDER_INFORATION));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Sender_device: ");
  Serial.println(XIAOS3_SENDER_INFORATION.Sender_device);
  Serial.print("Sender_Trag: ");
  Serial.println(XIAOS3_SENDER_INFORATION.Sender_Trag);
  Serial.println();
}
void ReceiverXIAOC6_Send_Data_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status){
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

void ReceiverXIAOC6_Send_Data(){
  
  strcpy(XIAOC6_RECEIVER_INFORATION.Reveiver_device, "XIAOC6"); 
  strcpy(XIAOC6_RECEIVER_INFORATION.Reveiver_Trag, "I'm get it"); 

  esp_err_t XIAOC6_RECEIVER_INFORATION_data1 = esp_now_send(XIAOS3_Sender_MAC_Address, (uint8_t *)&XIAOC6_RECEIVER_INFORATION, sizeof(receiver_meesage_types));

  if (XIAOC6_RECEIVER_INFORATION_data1 == ESP_OK)
  {
    Serial.println("Sent with success: XIAOC6_RECEIVER_INFORATION_data1");
  }
  delay(4000);
}

void Association_SenderXIAOS3_peer(){
  Serial.println("Attempting to associate peer for XIAOC6...");
  peerInfo_sender.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo_sender.encrypt = NO_PMK_KEY;

  memcpy(peerInfo_sender.peer_addr, XIAOS3_Sender_MAC_Address, 6);
  esp_err_t addPressStatus = esp_now_add_peer(&peerInfo_sender);
  if (addPressStatus != ESP_OK)
  {
    Serial.print("Failed to add peer");
    Serial.println(addPressStatus);
  }else
  {
    Serial.println("Successful to add peer");
  }
}