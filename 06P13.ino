const int PWM_PIN = 9;
int duty = 0, step = 5;

void setup() { pinMode(PWM_PIN, OUTPUT); }

void loop() {
  analogWrite(PWM_PIN, duty);
  duty += step;
  if (duty <= 0 || duty >= 255) step = -step;
  duty = constrain(duty, 0, 255);
  delay(30);
}
