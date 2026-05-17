#include <Adafruit_MotorShield.h>

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor* myMotor = AFMS.getMotor(1);

void setup() {
  Serial.begin(115200);

  while (!AFMS.begin()) {
    Serial.println("Could not find Motor Shield. Check wiring.");
    delay(1000);
  }

  Serial.println("Motor Shield found. Starting pump...");
  myMotor->setSpeed(255);
  myMotor->run(FORWARD);
}

void loop() {}
