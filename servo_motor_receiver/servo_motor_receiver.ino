// ================================================
// ESP-NOW 5-FINGER FLEX -> SERVO
// RECEIVER - 5 Servos (receives 0/90/180)

#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

const int SERVO_PINS[5] = {13, 12, 14, 27, 26};
Servo myServos[5];

typedef struct struct_message {
  uint8_t servoAngles[5];
} struct_message;

void printMACAddress() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("Receiver MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  struct_message incoming;
  memcpy(&incoming, incomingData, sizeof(incoming));
  
  for (int i = 0; i < 5; i++) {
    myServos[i].write(incoming.servoAngles[i]);
  }
  
  // Optional debug
  Serial.print("Servos moved: ");
  for (int i = 0; i < 5; i++) Serial.printf("%d ", incoming.servoAngles[i]);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  setCpuFrequencyMhz(80);
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
  
  printMACAddress();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  
  for (int i = 0; i < 5; i++) {
    myServos[i].attach(SERVO_PINS[i], 500, 2400);
    myServos[i].write(0);
  }
  
  Serial.println("=== 5-SERVO RECEIVER READY (0/90/180) ===");
}

void loop() {
  delay(1000);
}
