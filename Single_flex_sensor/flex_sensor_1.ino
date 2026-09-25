const int FLEX_PIN = 34;
const int SAMPLES = 30;
const float BASELINE_ALPHA = 0.004;
const int BEND_THRESH = 15;       // drop from recent peak to trigger state 1
const int DEBOUNCE_COUNT = 5;
const int RATE_WINDOW = 8;
const int CLIMB_RATE_THRESH = 4;
const int SLOW_CLIMB_THRESH = 2;
const int SLOW_DEBOUNCE = 20;
const int SETTLE_COUNT = 15;

float baseline = 800;
int state = 0;
int counter = 0;
int slowCounter = 0;
int settleCounter = 0;
int recentPeak = 0; // highest val seen recently in state 0
int valHistory[RATE_WINDOW];
int histIndex = 0;

int readFlexsensor() {
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(FLEX_PIN);
    delayMicroseconds(100);
  }
  return sum / SAMPLES;
}

int getStableReading() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += readFlexsensor();
    delay(2);
  }
  return sum / 10;
}

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  int initial = getStableReading();
  baseline = initial;
  recentPeak = initial;
  for (int i = 0; i < RATE_WINDOW; i++) valHistory[i] = initial;
}

void loop() {
  int val = getStableReading();
  int oldVal = valHistory[histIndex];
  int rate = val - oldVal;
  valHistory[histIndex] = val;
  histIndex = (histIndex + 1) % RATE_WINDOW;

  if (state == 0) {
    baseline = BASELINE_ALPHA * val + (1 - BASELINE_ALPHA) * baseline;
    
    // Track recent peak — slowly decays down so it doesn't get stuck high
    if (val > recentPeak) {
      recentPeak = val;       // instant update upward
    } else {
      recentPeak = 0.995 * recentPeak + 0.005 * val; // slow decay downward
    }

    int drop = recentPeak - val; // how far val has dropped from recent peak

    if (drop > BEND_THRESH) {
      counter++;
      if (counter >= DEBOUNCE_COUNT) {
        state = 1;
        counter = 0;
        slowCounter = 0;
        settleCounter = 0;
        for (int i = 0; i < RATE_WINDOW; i++) valHistory[i] = val;
        Serial.println(">> State 1: BENT");
      }
    } else {
      counter = 0;
    }
  } else {
    settleCounter++;
    if (settleCounter < SETTLE_COUNT) {
      for (int i = 0; i < RATE_WINDOW; i++) valHistory[i] = val;
    } else {
      // Fast return
      if (rate >= CLIMB_RATE_THRESH) {
        counter++;
        if (counter >= DEBOUNCE_COUNT) {
          state = 0;
          counter = 0;
          slowCounter = 0;
          recentPeak = val; // reset peak to current val on return
          Serial.println(">> State 0: REST (fast)");
        }
      } else {
        counter = 0;
      }

      // Slow return
      if (rate >= SLOW_CLIMB_THRESH) {
        slowCounter++;
        if (slowCounter >= SLOW_DEBOUNCE) {
          state = 0;
          counter = 0;
          slowCounter = 0;
          recentPeak = val;
          Serial.println(">> State 0: REST (slow)");
        }
      } else {
        slowCounter = 0;
      }

      // Fallback: gap from baseline
    }
  }

  Serial.print(val);
  Serial.print(" ");
  Serial.print(baseline);
  Serial.print(" ");
  Serial.print(state * 50 + 820);
  Serial.print(" ");
  Serial.println(rate);
}
