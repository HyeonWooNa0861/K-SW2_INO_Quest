// Arduino pin assignment
#define PIN_LED   9        // 반드시 PWM 핀
#define PIN_TRIG  12       // sonar TRIGGER
#define PIN_ECHO  13       // sonar ECHO

// configurable parameters
#define SND_VEL         346.0     // m/s @24°C
#define INTERVAL        25        // sampling interval [ms]
#define PULSE_DURATION  10        // trigger pulse [us]
#define _DIST_MIN       100.0     // min distance [mm]
#define _DIST_MAX       300.0     // max distance [mm]

// echo timeout (us) — 여유있게 설정
#define TIMEOUT   ((unsigned long)(INTERVAL) * 1000UL)
#define SCALE     (0.001 * 0.5 * SND_VEL)  // duration(us) → distance(mm)

unsigned long last_sampling_time = 0;   // [ms]

// ---- forward decl
float USS_measure(int TRIG, int ECHO);

// LED 극성: 액티브-로우(LOW=ON)이면 true, 일반이면 false
const bool INVERT_LED = true;   // 필요에 따라 false로

// PWM 출력 어댑터: 극성 자동 반전
static inline void writePWM(uint8_t val) {
  analogWrite(PIN_LED, INVERT_LED ? (uint8_t)(255 - val) : val);
}

void setup() {
  pinMode(PIN_LED,  OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(115200);
  last_sampling_time = millis();
}


void loop() {
  if (millis() - last_sampling_time < INTERVAL) return;
  last_sampling_time += INTERVAL;

  float distance = USS_measure(PIN_TRIG, PIN_ECHO);

  // 100~300mm에서 200mm가 최대 밝기
  uint8_t pwm = 0;
  if (distance >= _DIST_MIN && distance <= _DIST_MAX) {
    float amp = (distance <= 200.0f)
                ? (distance - 100.0f) / 100.0f      // 100→0, 200→1
                : (300.0f - distance) / 100.0f;     // 200→1, 300→0
    if (amp < 0) amp = 0;
    if (amp > 1) amp = 1;
    pwm = (uint8_t)(amp * 255.0f + 0.5f);
  } else {
    pwm = 0;
  }
//  writePWM(pwm);
  Serial.print("Min:");       Serial.print(_DIST_MIN);
  Serial.print(", distance:");Serial.print(distance, 1);
  Serial.print(", Max:");     Serial.print(_DIST_MAX);
  Serial.print(", pwm:");     Serial.println(pwm);

  analogWrite(PIN_LED, INVERT_LED ? (uint8_t)(255 - pwm) : pwm);

  // 로그
  Serial.print("Min:");       Serial.print(_DIST_MIN);
  Serial.print(", distance:");Serial.print(distance, 1);
  Serial.print(", Max:");     Serial.print(_DIST_MAX);
  Serial.print(", pwm:");     Serial.println(pwm);

}

// 초음파 거리(mm) 측정
float USS_measure(int TRIG, int ECHO) {
  // 트리거 펄스
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  // 왕복 시간(us) 측정 (타임아웃 포함)
  unsigned long dur = pulseIn(ECHO, HIGH, TIMEOUT);
  if (dur == 0UL) return 0.0;  // 타임아웃 시 0 반환

  // 거리(mm) = dur(us) * SCALE
  return (float)dur * (float)SCALE;
}
