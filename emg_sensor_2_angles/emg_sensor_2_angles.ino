// RC-A-056 EMG sensor -> ESP-NOW Transmitter
// Sends FIST (180°) or RELAXED (0°) to ALL 5 servos on the other ESP
// Same logic as your single-servo version (baseline calibration + spike detection)

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ================== EMG PIN ==================
const int emgPin = 34;

// ================== EMG SETTINGS ==================
int emgRaw = 0;
float emgSmoothed = 0.0;
float prevSmoothed = 0.0;
float baseline = 0.0;

float attack = 0.6;
float release = 0.05;

const float SPIKE_THRESHOLD = 15.0;

int decreaseCount = 0;
const int DECREASE_SAMPLES = 50;
const float RELAX_THRESHOLD = 1.25;

String gesture = "RELAXED";
String prevGesture = "";

// Timing
unsigned long previousMillis = 0;
const long interval = 10; // 100 Hz

// ================== ESP-NOW ==================
// REPLACE WITH YOUR RECEIVER MAC (printed by the servo ESP)
uint8_t receiverMAC[] = {0xF4, 0x2D, 0xC9, 0x71, 0xCF, 0x94};

typedef struct struct_message {
  uint8_t servoAngles[5];
} struct_message;

struct_message outgoing;

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

// ✅ FIXED: Updated callback signature for IDF v5.x
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  // Optional debug
  // Serial.print("ESP-NOW send: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  // ================== BASELINE CALIBRATION ==================
  Serial.println("=== CALIBRATING RELAXED BASELINE ===");
  Serial.println("Keep your arm completely relaxed for 5 seconds...");
  delay(2000);

  float sum = 0.0;
  for (int i = 0; i < 500; i++) {
    sum += analogRead(emgPin);
    delay(10);
  }
  baseline = sum / 500.0;
  emgSmoothed = baseline;

  Serial.print("Baseline calibrated -> ");
  Serial.print(baseline, 1);
  Serial.println(" (values near this = RELAXED)");

  // ================== ESP-NOW SETUP ==================
  WiFi.mode(WIFI_STA);
  setCpuFrequencyMhz(80);
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
  
  printMACAddress();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  // Add receiver as peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add receiver peer!");
    return;
  }

  Serial.println("EMG Transmitter Ready -> sending to 5-servo receiver");
  Serial.println("FIST = all servos 180° | RELAXED = all servos 0°");
  Serial.println("----------------------------------");

  // Initial relaxed state
  gesture = "RELAXED";
  prevGesture = "";
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // 1. Read raw EMG
    emgRaw = analogRead(emgPin);

    // 2. Envelope follower
    prevSmoothed = emgSmoothed;
    if (emgRaw > emgSmoothed)
      emgSmoothed = emgSmoothed * (1 - attack) + emgRaw * attack;
    else
      emgSmoothed = emgSmoothed * (1 - release) + emgRaw * release;

    if (emgSmoothed < 1.0) emgSmoothed = 0.0;

    // 3. Delta
    float delta = emgSmoothed - prevSmoothed;

    // 4. FIST detection
    if (delta > SPIKE_THRESHOLD) {
      gesture = "FIST";
      decreaseCount = 0;
    }
    // 5. RELAXED: near baseline OR long continuous decrease
    else if (emgSmoothed <= baseline * RELAX_THRESHOLD || decreaseCount >= DECREASE_SAMPLES) {
      gesture = "RELAXED";
      decreaseCount = 0;
    }
    // 6. Counting slow decrease
    else if (delta < 0) {
      decreaseCount++;
      if (decreaseCount >= DECREASE_SAMPLES) {
        gesture = "RELAXED";
      }
    } else {
      decreaseCount = 0;
    }

    // 7. SEND only when gesture changes (no spam)
    if (gesture != prevGesture) {
      for (int i = 0; i < 5; i++) {
        outgoing.servoAngles[i] = (gesture == "FIST") ? 180 : 0;
      }
      esp_now_send(receiverMAC, (uint8_t *)&outgoing, sizeof(outgoing));

      if (gesture == "FIST") {
        Serial.println(">>> FIST detected - Sent 180° to all 5 servos");
      } else {
        Serial.println(">>> RELAXED detected - Sent 0° to all 5 servos");
      }
      prevGesture = gesture;
    }

    // 8. Serial tuning output
    Serial.print("Raw: ");
    Serial.print(emgRaw);
    Serial.print(" | Env: ");
    Serial.print(emgSmoothed, 1);
    Serial.print(" | Delta: ");
    Serial.print(delta, 2);
    Serial.print(" | DecCount: ");
    Serial.print(decreaseCount);
    Serial.print(" | Baseline: ");
    Serial.print(baseline, 1);
    Serial.print(" | Gesture: ");
    Serial.println(gesture);
  }
}
