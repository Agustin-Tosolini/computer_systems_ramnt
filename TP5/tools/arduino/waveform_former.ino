const int gpioA = 2;
const int gpioB = 3;

void setup() {
  pinMode(gpioA, OUTPUT);
  pinMode(gpioB, OUTPUT);
}

void loop() {
  // 00
  digitalWrite(gpioA, LOW);
  digitalWrite(gpioB, LOW);
  delay(500);

  // 01
  digitalWrite(gpioA, LOW);
  digitalWrite(gpioB, HIGH);
  delay(500);

  // 10
  digitalWrite(gpioA, HIGH);
  digitalWrite(gpioB, LOW);
  delay(500);

  // 11
  digitalWrite(gpioA, HIGH);
  digitalWrite(gpioB, HIGH);
  delay(500);
}