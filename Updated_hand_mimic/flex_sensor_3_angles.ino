// ================================================
// ESP-NOW 5-FINGER FLEX -> SERVO
// TRANSMITTER - EXACTLY your latest single-finger logic
// Calibration: only Flat (0°) + Fully Bent (180°)
// snapAngle uses t1=60% and t2=90% exactly as in your code
// Controls 5 fingers -> 5 servos wirelessly
// ================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ====================== SHARED ======================
const int FLEX_PINS[5] = {34, 35, 36, 39, 32}; // Finger 1 to 5
const int LED_PIN = 2;
const int SAMPLES = 32;
const float EMA_ALPHA = 0.15f;

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

// ====================== FINGER CLASS (exact same logic as your single finger) ======================
class FlexFinger {
private:
  int pin;
  int fingerNum;
  int flatVal = 0;
  int bentVal = 0;
  float ema = 0.0f;
  int currentAngle = 0;
public:
  FlexFinger(int p, int num) : pin(p), fingerNum(num) {}
  
  void calibrate(int flat, int bent) {
    flatVal = flat;
    bentVal = bent;
    ema = stableRead(pin);
    Serial.printf("Finger %d -> Flat(0°):%d Bent(180°):%d\n", fingerNum, flatVal, bentVal);
  }
  
  void update() {
    int raw = stableRead(pin);
    ema = EMA_ALPHA * raw + (1.0f - EMA_ALPHA) * ema;
    int smoothed = (int)ema;
    
    // EXACT snapAngle from your code
    int t1 = flatVal + (bentVal - flatVal) * 0.60f; // 0° -> 90° boundary
    int t2 = flatVal + (bentVal - flatVal) * 0.83f; // 90° -> 180° boundary
    
    if (smoothed > t1)      currentAngle = 0;
    else if (smoothed > t2) currentAngle = 90;
    else                    currentAngle = 180;
  }
  
  int getAngle() const { return currentAngle; }
  int getSmoothed() const { return (int)ema; }
};

// ====================== sensor INSTANCES ======================
FlexFinger finger[5] = {
  FlexFinger(34, 1),
  FlexFinger(35, 2),
  FlexFinger(36, 3),
  FlexFinger(39, 4),
  FlexFinger(32, 5)
};

// ====================== ESP-NOW ======================
typedef struct struct_message {
  uint8_t servoAngles[5];
} struct_message;

struct_message outgoingData;
uint8_t lastSentAngles[5] = {0};

uint8_t receiverMacAddress[] = {0xF4, 0x2D, 0xC9, 0x71, 0xCF, 0x94}; // <--- CHANGE TO YOUR RECEIVER MAC

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

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  pinMode(LED_PIN, OUTPUT);

  // Battery saving
  WiFi.mode(WIFI_STA);
  setCpuFrequencyMhz(80);
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
  
  printMACAddress();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  
  Serial.println("\n=== 5-FINGER CALIBRATION (exact same as your single finger) ===");
  
  // STEP 1: All fingers STRAIGHT (0°)
  Serial.println("HOLD ALL 5 FINGERS STRAIGHT (0 deg)... 3 seconds");
  digitalWrite(LED_PIN, HIGH);
  delay(3000);
  
  int flatVals[5];
  for (int i = 0; i < 5; i++) {
    flatVals[i] = stableRead(FLEX_PINS[i]);
    Serial.printf("Finger %d Flat: %d\n", i+1, flatVals[i]);
  }
  
  digitalWrite(LED_PIN, LOW);
  blinkLED(2, 200);
  delay(500);
  
  // STEP 2: All fingers FULLY BENT (180°)
  Serial.println("BEND ALL 5 FINGERS FULLY (180 deg)... 3 seconds");
  for (int i = 0; i < 15; i++) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW);  delay(100);
  }
  
  int bentVals[5];
  for (int i = 0; i < 5; i++) {
    bentVals[i] = stableRead(FLEX_PINS[i]);
    Serial.printf("Finger %d Bent: %d\n", i+1, bentVals[i]);
  }
  
  blinkLED(5, 100);
  
  // Calibrate all fingers
  for (int i = 0; i < 5; i++) {
    finger[i].calibrate(flatVals[i], bentVals[i]);
  }
  
  Serial.println("\n=== CALIBRATION DONE - Running ===\n");
  Serial.println("Logic: 0° (large) | 90° (narrow) | 180° (small) exactly like your code");
}

void loop() {
  for (int i = 0; i < 5; i++) {
    finger[i].update();
  }
  
  uint8_t newAngles[5];
  for (int i = 0; i < 5; i++) {
    newAngles[i] = finger[i].getAngle();
  }
  
  // Send only when any finger changes
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
    for (int i = 0; i < 5; i++) Serial.printf("%d ", newAngles[i]);
    Serial.println();
  }
  
  // LED feedback on board (uses Finger 1 exactly like your single-finger code)
  int ledAngle = finger[0].getAngle();
  if (ledAngle == 0)      digitalWrite(LED_PIN, LOW);
  else if (ledAngle == 180) digitalWrite(LED_PIN, HIGH);
  else                      digitalWrite(LED_PIN, HIGH); // Just keep it ON at 90°
  
  // Serial Plotter: smoothed values for all 5 fingers
  for (int i = 0; i < 5; i++) {
    Serial.print(finger[i].getSmoothed());
    Serial.print(" ");
  }
  Serial.println();
  
  delay(50);
}
