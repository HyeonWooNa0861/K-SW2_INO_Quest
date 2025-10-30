#include <Servo.h>

// ---------------- Servo 설정 ----------------
#define PIN_SERVO 10

// Servo 펄스 한계 (0° ↔ 180°)
#define _DUTY_MIN 500     // 서보 0도
#define _DUTY_MAX 2500    // 서보 180도

// 제어 속도 및 주기
#define _SERVO_SPEED 0.3f   // 각속도 (deg/sec)
#define INTERVAL      20.0f // 제어 주기 (ms)

// ---------------- 전역 변수 ----------------
Servo myservo;
unsigned long last_ms = 0;     // 마지막 루프 시간 (ms)

// 현재 상태
float duty_curr;               // 현재 펄스폭 (µs)
float duty_target;             // 목표 펄스폭 (µs)
float duty_step;               // 한 주기당 변화량 (µs)
bool  moving_up = true;        // 현재 방향 (true: 0→180, false: 180→0)

void setup() {
  Serial.begin(115200);
  myservo.attach(PIN_SERVO);

  // 초기 위치
  duty_curr = _DUTY_MIN;
  duty_target = _DUTY_MAX;
  myservo.writeMicroseconds((int)duty_curr);

  // 펄스/도 계산
  const float us_per_deg = (float)(_DUTY_MAX - _DUTY_MIN) / 180.0f;
  duty_step = _SERVO_SPEED * us_per_deg * (INTERVAL / 1000.0f);

  // 디버그용 출력
  Serial.println("=== Servo Motion Info ===");
  Serial.print("us_per_deg: "); Serial.println(us_per_deg, 3);
  Serial.print("duty_step: "); Serial.println(duty_step, 4);
  Serial.print("speed: "); Serial.print(_SERVO_SPEED); Serial.println(" deg/sec");

  last_ms = millis();
}

void loop() {
  // ---- 일정한 주기로 제어 ----
  if (millis() - last_ms < INTERVAL) return;
  last_ms = millis();

  // ---- 서보 위치 갱신 ----
  if (moving_up) {
    duty_curr += duty_step;
    if (duty_curr >= _DUTY_MAX) {
      duty_curr = _DUTY_MAX;
      moving_up = false;           // 방향 반전
      duty_target = _DUTY_MIN;     // 바로 반전
      Serial.println("↘ REVERSE: moving DOWN");
    }
  } else {
    duty_curr -= duty_step;
    if (duty_curr <= _DUTY_MIN) {
      duty_curr = _DUTY_MIN;
      moving_up = true;            // 방향 반전
      duty_target = _DUTY_MAX;     // 바로 반전
      Serial.println("↗ REVERSE: moving UP");
    }
  }

  // ---- 서보 출력 ----
  myservo.writeMicroseconds((int)duty_curr);

  // ---- 1초마다 상태 출력 ----
  static int serial_cnt = 0;
  if (++serial_cnt >= (1000 / INTERVAL)) {  // INTERVAL=20ms → 50틱≈1초
    serial_cnt = 0;
    Serial.print("curr(us)="); Serial.print(duty_curr, 1);
    Serial.print(", target(us)="); Serial.print(duty_target, 1);
    Serial.print(", step(us)="); Serial.print(duty_step, 3);
    Serial.print(", dir="); Serial.println(moving_up ? "UP" : "DOWN");
  }
}
