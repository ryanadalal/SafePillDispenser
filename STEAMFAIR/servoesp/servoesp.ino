#include <ESP32Servo.h>
Servo myservo;  // create servo object to control a servo


void setup() {
  myservo.attach(18);  // attaches the servo on pin 9 to the servo object
}

void loop() {
  myservo.writeMicroseconds(1000);
  delay(5000);
  myservo.writeMicroseconds(1500);
  delay(5000);
  myservo.writeMicroseconds(2000);
  delay(5000);
  
}