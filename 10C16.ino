#include <Servo.h>
#include <math.h>

// ---------------- Pins ----------------
#define PIN_TRIG   12
#define PIN_ECHO   13
#define PIN_SERVO  10

// ---------------- 초음파 설정 ----------------
#define SND_VEL          346.0f
#define SAMPLE_INTERVAL  60        // [ms]
#define PULSE_US         20        // TRIG 펄스 [us]
#define TIMEOUT_US       50000UL   // Echo 타임아웃 [us]
#define SCALE            (0.001f * 0.5f * SND_VEL)  // us→mm

// 감지 히스테리시스(mm)
#define DETECT_ON_MM     200.0f    // 20cm 이하면 OPEN
#define DETECT_OFF_MM    350.0f    // 35cm 이상이면 CLOSE

// ---------------- EMA 필터 ----------------
#define EMA_ALPHA        0.7f
float distEMA = 0.0f;
bool  emaInit = false;

// ---------------- Servo ----------------
Servo gate;

// 펄스 폭 범위(하드웨어 보정)
const int PULSE_MIN = 1000;         // ≈ 0°
const int PULSE_MAX = 3000;        // ≈ 180°
static inline int angleToPulse(float deg){
  deg = constrain(deg, 0, 180);
  float n = deg / 180.0f;
  return (int)(PULSE_MIN + (PULSE_MAX - PULSE_MIN) * n + 0.5f);
}
static inline void writeAngle(float deg){
  gate.writeMicroseconds(angleToPulse(deg));
}

// 각도 정의
const float CLOSED_ANGLE = 0.0f;
const float OPEN_ANGLE   = 90.0f;

// ── Slew 리미터 파라미터 ──
const unsigned long UPDATE_DT_MS = 20;   // 50Hz
const float MAX_DEG_PER_SEC = 60.0f;     // 느리게 하려면 ↓

// ── 자동 닫힘 ──
const unsigned long AUTOCLOSE_MS = 2000; // 감지 끊긴 뒤 2초 후 닫힘
unsigned long lastDetectMs = 0;

// ---------------- 상태 ----------------
unsigned long lastSampleMs = 0;
unsigned long lastUpdateMs = 0;

bool  detected = false;          // 차량 감지 상태
float currentAngle = CLOSED_ANGLE;
float targetAngle  = CLOSED_ANGLE;

// ---------------- 함수 ----------------
static inline float measure_mm(){
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(PULSE_US);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long dur = pulseIn(PIN_ECHO, HIGH, TIMEOUT_US);
  if (dur == 0UL) return 0.0f;
  return (float)dur * SCALE;
}

void setup(){
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  gate.attach(PIN_SERVO);        // 범위 지정 X (이중 매핑 방지)
  writeAngle(currentAngle);

  Serial.begin(115200);
  lastSampleMs = lastUpdateMs = millis();
  lastDetectMs = millis();
}

void loop(){
  unsigned long now = millis();

  // 1) 초음파 측정 + EMA
  if (now - lastSampleMs >= SAMPLE_INTERVAL){
    lastSampleMs += SAMPLE_INTERVAL;

    float raw = measure_mm();
    if (!emaInit){ distEMA = raw; emaInit = true; }
    else          distEMA = EMA_ALPHA*raw + (1.0f-EMA_ALPHA)*distEMA;

    // 히스테리시스 감지
    if (!detected && distEMA > 0.0f && distEMA <= DETECT_ON_MM){
      detected    = true;
      targetAngle = OPEN_ANGLE;            // 열림 목표
      lastDetectMs = now;                  // 마지막 감지 시각 업데이트
      Serial.println("🚗 DETECT → target=OPEN");
    } 
    else if (detected && (distEMA == 0.0f || distEMA >= DETECT_OFF_MM)){
      detected = false;
      // 여기서 즉시 닫지 않고, 자동닫힘 타이머로 맡김
      Serial.println("⬅️ CLEAR (auto-close timer starts)");
    }

    // 감지 유지 중이면 타임스탬프 갱신
    if (detected) lastDetectMs = now;

    // 자동 닫힘: 감지 끊긴 뒤 2초 지나면 닫힘 명령
    if (!detected && targetAngle == OPEN_ANGLE && (now - lastDetectMs >= AUTOCLOSE_MS)){
      targetAngle = CLOSED_ANGLE;
      Serial.println("⏱ Auto-CLOSE triggered");
    }

    // 디버그
    Serial.print("raw=");  Serial.print(raw,1);
    Serial.print("  ema=");Serial.print(distEMA,1);
    Serial.print("  det=");Serial.print(detected);
    Serial.print("  cur=");Serial.print(currentAngle,1);
    Serial.print("  tgt=");Serial.println(targetAngle,1);
  }

  // 2) Slew 리미터만으로 모션 수행 (선형 속도)
  if (now - lastUpdateMs >= UPDATE_DT_MS){
    lastUpdateMs = now;

    float maxStep = MAX_DEG_PER_SEC * (UPDATE_DT_MS / 1000.0f); // 틱당 최대 이동각
    float delta   = targetAngle - currentAngle;

    if      (delta >  maxStep) delta =  maxStep;
    else if (delta < -maxStep) delta = -maxStep;

    currentAngle += delta;

    // 목표에 매우 가까우면 스냅(잔떨림 방지)
    if (fabs(targetAngle - currentAngle) < 0.1f) currentAngle = targetAngle;

    writeAngle(currentAngle);
  }
}
