const int LED_PIN = 7;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // 시작 시 OFF
}

void loop() {
  digitalWrite(LED_PIN, HIGH); // 1초 켬
  delay(1000);

  // 1초 동안 5번 깜빡(200ms * 5)
  for (int i = 0; i < 5; ++i) {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
  }

  digitalWrite(LED_PIN, LOW00.);  // 정지 상태: 끔
  while (1) { }                // 무한 대기 (탈출 안 함)
}
