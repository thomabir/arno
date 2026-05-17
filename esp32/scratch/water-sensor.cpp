#include <Arduino.h>

void setup() {
  Serial.begin(9600);
  pinMode(5, INPUT);
}

void loop() {
  int water_detected = 0;
  water_detected = digitalRead(5);
  Serial.print("Water detected: ");
  Serial.println(water_detected, DEC);
  delay(500);
}