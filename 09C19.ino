// Arduino pin assignment
#define PIN_LED   9
#define PIN_TRIG  12
#define PIN_ECHO  13

// configurable parameters
#define SND_VEL         346.0     // m/s @24°C
#define INTERVAL        25        // sampling interval [ms]
#define PULSE_DURATION  10        // trigger pulse [us]
#define _DIST_MIN       100.0     // min distance [mm]
#define _DIST_MAX       300.0     // max distance [mm]

// echo timeout
#define TIMEOUT   ((unsigned long)(INTERVAL) * 1000UL)
#define SCALE     (0.001 * 0.5 * SND_VEL)  // duration(us) → distance(mm)

unsigned long last_sampling_time = 0;   // [ms]

// ---- Median filter ----
#define N 10  // window size (홀수 권장)
float buffer[N];
int   buf_index = 0;
bool  buf_filled = false;

float medianFilter(float v) {
  buffer[buf_index] = v;
  buf_index = (buf_index + 1) % N;
  if (buf_index == 0) buf_filled = true;

  int count = buf_filled ? N : buf_index;
  float temp[N];
  for (int i = 0; i < count; ++i) temp[i] = buffer[i];

  // 작은 N에 적합한 삽입정렬
  for (int i = 1; i < count; ++i) {
    float key = temp[i];
    int j = i - 1;
    while (j >= 0 && temp[j] > key) { temp[j + 1] = temp[j]; --j; }
    temp[j + 1] = key;
  }
  return temp[count / 2];
}

// ---- EMA ----
#define _EMA_ALPHA  0.3f           // 0~1 (작을수록 더 매끈/느림)
float dist_ema = 0.0f;
bool  ema_inited = false;

// ---- USS ----
float USS_measure(int TRIG, int ECHO) {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  unsigned long dur = pulseIn(ECHO, HIGH, TIMEOUT);
  if (dur == 0UL) return 0.0f;     // timeout
  return (float)dur * (float)SCALE;
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(115200);
  last_sampling_time = millis();
}

void loop() {
  if (millis() - last_sampling_time < INTERVAL) return;
  last_sampling_time += INTERVAL;

  // 1) 원본 측정값
  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // 2) 중위수 필터
  float dist_median = medianFilter(dist_raw);

  // 3) EMA (median 기반으로 더 매끈하게)
  if (!ema_inited) { dist_ema = dist_median; ema_inited = true; }
  else {
    dist_ema = _EMA_ALPHA * dist_median + (1.0f - _EMA_ALPHA) * dist_ema;
    // 만약 raw 기반 EMA를 원하면 위에서 dist_median 대신 dist_raw 사용
  }

  // 예시 제어: median 기준으로 100~300mm이면 ON
  digitalWrite(PIN_LED, (dist_median > 100.0 && dist_median < 300.0) ? LOW : HIGH);

  // ── 시리얼 플로터 출력형식 ──
  Serial.print("Min:");      Serial.print(_DIST_MIN);
  Serial.print(", raw:");    Serial.print(dist_raw);
  Serial.print(", ema:");    Serial.print(dist_ema);
  Serial.print(", median:"); Serial.print(dist_median);
  Serial.print(", Max:");    Serial.print(_DIST_MAX);
  Serial.println("");
}
