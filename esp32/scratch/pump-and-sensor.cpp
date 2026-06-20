#include <Adafruit_MotorShield.h>
#include <Arduino.h>

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor* myMotor = AFMS.getMotor(1);

void setup() {
  Serial.begin(115200);

  // water sensor
  pinMode(5, INPUT);

  // pump motor
  while (!AFMS.begin()) {
    Serial.println("Could not find Motor Shield. Check wiring.");
    delay(1000);
  }
}

void loop() {
  // Read water sensor
  int water_detected = digitalRead(5);

  // Pump if no water detected, otherwise stop pump
  if (water_detected == LOW) {
    myMotor->setSpeed(255);
    myMotor->run(FORWARD);
    Serial.println("Pumping water...");
  } else {
    myMotor->run(RELEASE);
    Serial.println("Water detected, stopping pump.");
  }
  delay(500);
}