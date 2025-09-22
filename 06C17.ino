const int LED_PIN = 9;  // LED 핀 번호

// ─ PWM 설정 ─
const unsigned int PERIOD_US = 100;  // 주기 (2ms = 500Hz)
const int DUTY_PERCENT = 100;  // 듀티 비율 (%)
int i = 0;
int step = 1;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  i += step;

  if (i <= 0 || i >= DUTY_PERCENT) {
    step = -step;
  }
  
  // 켜짐 시간 = duty% * period
  unsigned int on_us = (unsigned long)i * PERIOD_US / 100;

  // 1) LED ON
  if (on_us > 0) {
    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(on_us);
//    printf(on_us);
  }

  // 2) LED OFF
  if (on_us < PERIOD_US) {
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(PERIOD_US - on_us);
//    printf(PERIOD_US - on_us);
  }
}
