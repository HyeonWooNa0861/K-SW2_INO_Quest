#include <Servo.h>

// ---------------- Pin 설정 ----------------
#define PIN_IR    A0
#define PIN_LED   9
#define PIN_SERVO 10

// ---------------- 거리 및 서보 설정 ----------------
#define _DIST_MIN  100.0f
#define _DIST_MAX  250.0f
#define _DUTY_MIN  500
#define _DUTY_MAX  2500

// ---------------- 필터 설정 ----------------
#define EMA_ALPHA  0.5f
#define MEDIAN_N   5

// ---------------- 루프 주기 ----------------
#define LOOP_INTERVAL   20
#define SERIAL_INTERVAL 200

// ---------------- 전역 변수 ----------------
Servo myservo;
unsigned long last_loop_time = 0;
unsigned long last_serial_ms = 0;   // ✅ 추가됨

float samples[MEDIAN_N];
int sample_index = 0;

float dist_prev = _DIST_MIN;
float dist_ema  = _DIST_MIN;
float last_IR_value = 0.0f;         // ✅ a_value 저장용

// ---------------- 함수 정의 ----------------
float measureIR() {
  last_IR_value = analogRead(PIN_IR);   // ✅ 전역 저장
  return ((6762.0 / (last_IR_value - 9.0)) - 4.0) * 10.0;
}

float medianFilter(float newVal) {
  samples[sample_index++] = newVal;
  if (sample_index >= MEDIAN_N) sample_index = 0;

  float sorted[MEDIAN_N];
  memcpy(sorted, samples, sizeof(sorted));

  for (int i = 0; i < MEDIAN_N - 1; i++) {
    for (int j = i + 1; j < MEDIAN_N; j++) {
      if (sorted[i] > sorted[j]) {
        float tmp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = tmp;
      }
    }
  }
  return sorted[MEDIAN_N / 2];
}

float distanceToDuty(float dist) {
  if (dist < _DIST_MIN) dist = _DIST_MIN;
  if (dist > _DIST_MAX) dist = _DIST_MAX;
  return _DUTY_MIN + (dist - _DIST_MIN) * (_DUTY_MAX - _DUTY_MIN) / (_DIST_MAX - _DIST_MIN);
}

// ---------------- setup() ----------------
void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds((_DUTY_MIN + _DUTY_MAX) / 2);

  Serial.begin(1000000); // 필요시 115200으로 낮춰 테스트
  for (int i = 0; i < MEDIAN_N; i++) samples[i] = _DIST_MIN;
}

// ---------------- loop() ----------------
void loop() {
  unsigned long now = millis();
  if (now < last_loop_time + LOOP_INTERVAL) return;
  last_loop_time += LOOP_INTERVAL;

  float dist_raw = measureIR();
  float dist_median = medianFilter(dist_raw);

  float dist_filtered;
  if (dist_median < _DIST_MIN || dist_median > _DIST_MAX) {
    dist_filtered = dist_prev;
    digitalWrite(PIN_LED, LOW);
  } else {
    dist_filtered = dist_median;
    dist_prev = dist_median;
    digitalWrite(PIN_LED, HIGH);
  }

  dist_ema = EMA_ALPHA * dist_filtered + (1.0f - EMA_ALPHA) * dist_ema;

  float duty = distanceToDuty(dist_ema);
  myservo.writeMicroseconds((int)duty);

  // ---- 시리얼 출력 주기 제어 ----
  if (now - last_serial_ms >= SERIAL_INTERVAL) {
    last_serial_ms = now;
    Serial.print("_DUTY_MIN:");  Serial.print(_DUTY_MIN);
    Serial.print("_DIST_MIN:");  Serial.print(_DIST_MIN);
    Serial.print(", IR:");       Serial.print(last_IR_value);
    Serial.print(", dist_raw:"); Serial.print(dist_raw, 1);
    Serial.print(", EMA:");      Serial.print(dist_ema, 1);
    Serial.print(", Servo:");    Serial.print(duty, 1);
    Serial.print(", _DIST_MAX:");Serial.print(_DIST_MAX);
    Serial.print(", _DUTY_MAX:");Serial.print(_DUTY_MAX);
    Serial.println("");
  }
}
