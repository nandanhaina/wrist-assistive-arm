// ================================================
// ESP-NOW 5-FINGER FLEX -> SERVO (Your Calibration Logic)
// TRANSMITTER - 5 Flex sensors
// ================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

const int FLEX_PINS[5] = {34, 35, 36, 39, 32}; // Change if needed
const int LED_PIN      = 2;
const int SAMPLES      = 32;
const float EMA_ALPHA  = 0.15f;

int FLAT_VAL[5]  = {0};
int BENT_VAL[5]  = {0};
int THRESHOLD[5] = {0};
float ema[5]     = {0};

int stableRead(int pin) {
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(pin);
    delayMicroseconds(200);
  }
  return sum / SAMPLES;
}

void blinkLED(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH); delay(ms);
    digitalWrite(LED_PIN, LOW);  delay(ms);
  }
}

void printMACAddress() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("Transmitter MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

typedef struct struct_message {
  uint8_t servoAngles[5];
} struct_message;

struct_message outgoingData;
uint8_t lastSentAngles[5] = {0};

uint8_t receiverMacAddress[] = {0xF4, 0x2D, 0xC9, 0x71, 0xCF, 0x94}; // <--- CHANGE TO YOUR RECEIVER MAC

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  pinMode(LED_PIN, OUTPUT);

  // === BATTERY SAVING ===
  WiFi.mode(WIFI_STA);
  setCpuFrequencyMhz(80);
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);

  printMACAddress();

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("\n=== 5-FINGER CALIBRATION START ===");

  // STEP 1: All fingers STRAIGHT
  Serial.println("HOLD ALL 5 FINGERS STRAIGHT... 3 seconds");
  digitalWrite(LED_PIN, HIGH);
  delay(3000);
  for (int i = 0; i < 5; i++) {
    FLAT_VAL[i] = stableRead(FLEX_PINS[i]);
    Serial.printf("Finger %d Flat: %d\n", i+1, FLAT_VAL[i]);
  }
  digitalWrite(LED_PIN, LOW);
  blinkLED(2, 200);
  delay(500);

  // STEP 2: All fingers FULLY BENT
  Serial.println("BEND ALL 5 FINGERS FULLY... 3 seconds");
  for (int i = 0; i < 15; i++) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW);  delay(100);
  }
  for (int i = 0; i < 5; i++) {
    BENT_VAL[i] = stableRead(FLEX_PINS[i]);
    Serial.printf("Finger %d Bent: %d\n", i+1, BENT_VAL[i]);
  }
  blinkLED(5, 100);

  // Calculate thresholds
  for (int i = 0; i < 5; i++) {
    THRESHOLD[i] = (FLAT_VAL[i] + BENT_VAL[i]) / 2;
    ema[i] = stableRead(FLEX_PINS[i]); // initial EMA
    Serial.printf("Finger %d Threshold: %d\n", i+1, THRESHOLD[i]);
  }

  Serial.println("\n=== CALIBRATION DONE - Running ===\n");
}

void loop() {
  uint8_t newAngles[5];

  for (int i = 0; i < 5; i++) {
    int raw = stableRead(FLEX_PINS[i]);
    ema[i] = EMA_ALPHA * raw + (1.0f - EMA_ALPHA) * ema[i];
    int smoothed = (int)ema[i];
    
    newAngles[i] = (smoothed < THRESHOLD[i]) ? 180 : 0;
  }

  // Send only if anything changed
  bool changed = false;
  for (int i = 0; i < 5; i++) {
    if (newAngles[i] != lastSentAngles[i]) {
      changed = true;
      break;
    }
  }

  if (changed) {
    memcpy(outgoingData.servoAngles, newAngles, 5);
    esp_now_send(receiverMacAddress, (uint8_t*)&outgoingData, sizeof(outgoingData));
    memcpy(lastSentAngles, newAngles, 5);
    
    Serial.print("Sent -> ");
    for (int i = 0; i < 5; i++) Serial.print(newAngles[i] == 180 ? "B" : "R");
    Serial.println();
  }

  // Optional live plot (comment out to save more power)
  for (int i = 0; i < 5; i++) {
    Serial.printf("%d ", (int)ema[i]);
  }
  Serial.println();

  delay(50); // 20 Hz - very responsive
}
