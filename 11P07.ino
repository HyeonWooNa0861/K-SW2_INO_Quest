#include <Servo.h>
#include <math.h>

// -------- Pins --------
#define PIN_TRIG   12
#define PIN_ECHO   13
#define PIN_SERVO  10
#define PIN_LED     9

// -------- 초음파 설정 --------
#define SND_VEL          346.0f
#define SAMPLE_INTERVAL  60        // [ms]
#define PULSE_US         20        // TRIG HIGH 폭 [us]
#define TIMEOUT_US       50000UL   // echo 타임아웃 [us]
#define SCALE            (0.001f * 0.5f * SND_VEL)  // us → mm

// -------- 거리 → 각도 선형 매핑 구간 --------
#define DIST_MIN_MM      180.0f    // 180mm → ANGLE_MIN
#define DIST_MAX_MM      360.0f    // 360mm → ANGLE_MAX

// -------- 각도 범위 --------
#define ANGLE_MIN        0.0f      // 닫힘
#define ANGLE_MAX        180.0f    // 열림

// -------- EMA --------
#define EMA_ALPHA        0.4f
float distEMA = 0.0f;
bool  emaInit = false;

// -------- Servo --------
Servo gate;
// 서보 펄스 보정(필요시 600/2400 등으로 조정)
const int PULSE_MIN = 500;
const int PULSE_MAX = 2500;

// -------- 상태 --------
float currentAngle = ANGLE_MIN;    // 모니터 표시용
float targetAngle  = ANGLE_MIN;    // 즉시 출력 각
unsigned long lastSampleMs = 0;

// ---------- 유틸 ----------
static inline float clampf(float x, float a, float b){
  return (x < a) ? a : (x > b) ? b : x;
}
static inline float mapLinear(float x, float inMin, float inMax, float outMin, float outMax){
  if (inMax == inMin) return outMin;       // 0으로 나누기 방지
  float t = (x - inMin) / (inMax - inMin); // 0~1
  return outMin + t * (outMax - outMin);
}
static inline float measure_mm() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);   // 안정화
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(PULSE_US);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long dur = pulseIn(PIN_ECHO, HIGH, TIMEOUT_US);
  if (dur == 0UL) return 0.0f;                           // 타임아웃 → 0
  return (float)dur * SCALE;                             // mm
}
static inline int angleToPulse(float deg){
  deg = clampf(deg, 0.0f, 180.0f);
  float n = deg / 180.0f;
  return (int)(PULSE_MIN + (PULSE_MAX - PULSE_MIN) * n + 0.5f);
}
static inline void writeAngle(float deg){
  gate.writeMicroseconds(angleToPulse(deg));
}

// ---------- setup ----------
void setup(){
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED,  OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  digitalWrite(PIN_LED,  LOW);

  gate.attach(PIN_SERVO);
  writeAngle(currentAngle);

  Serial.begin(115200);
  lastSampleMs = millis();
}

// ---------- loop ----------
void loop(){
  unsigned long now = millis();

  // 1) 주기적으로 측정/계산
  if (now - lastSampleMs >= SAMPLE_INTERVAL){
    lastSampleMs += SAMPLE_INTERVAL;

    // 거리 측정 + EMA
    float dist_raw = measure_mm();
    if (!emaInit){ distEMA = dist_raw; emaInit = true; }
    else          { distEMA = EMA_ALPHA * dist_raw + (1.0f - EMA_ALPHA) * distEMA; }

    // 거리 → 각도 선형 매핑 (클램프 포함)
    float dClamped = clampf(distEMA, DIST_MIN_MM, DIST_MAX_MM);
    targetAngle = mapLinear(dClamped, DIST_MIN_MM, DIST_MAX_MM, ANGLE_MIN, ANGLE_MAX);

    // LED: 유효 구간(180~360mm)에서만 ON
    bool inRange = (distEMA >= DIST_MIN_MM && distEMA <= DIST_MAX_MM);
    digitalWrite(PIN_LED, inRange ? LOW : HIGH);

    // Slew 해제: 즉시 출력
    writeAngle(targetAngle);
    currentAngle = targetAngle;   // 모니터 표시용 동기화

    // Serial Plotter 형식 출력
    Serial.print("Min:");     Serial.print(DIST_MIN_MM);
    Serial.print(",dist:");   Serial.print(dist_raw, 1);
    Serial.print(",ema:");    Serial.print(distEMA, 1);
    Serial.print(",Servo:");  Serial.print(currentAngle, 1);
    Serial.print(",Max:");    Serial.print(DIST_MAX_MM);
    Serial.println("");
  }
}
