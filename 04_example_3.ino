#define PIN_LED 13
unsigned int count = 0;
unsigned int toggle = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  while(!Serial) {
    ;
  }
  Serial.println("PIN_LED: ");
  Serial.println(PIN_LED);
  Serial.println("toggle: ");
  Serial.println(toggle);
}

void loop() {
  Serial.println(++count);
  toggle = toggle_state(toggle); // toggle LED value.
  digitalWrite(PIN_LED, toggle); // update LED status
  
  delay(1000);
}

int toggle_state(int toggle) {
  return !toggle; // 0 <-> switching
}
