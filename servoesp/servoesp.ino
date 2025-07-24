#include <ESP32Servo.h>
Servo myservo;  // create servo object to control a servo
Servo s2;

void setup() {
  myservo.attach(12);  // attaches the servo on pin 9 to the servo object
  Serial.begin(57600);
}

void loop() {
  Serial.println("Time");
    
  uint16_t fid = 0;

  while (! Serial.available());
  fid = Serial.parseInt();

  myservo.writeMicroseconds(1600);
  delay(fid);
  myservo.writeMicroseconds(1500);
    
  while (Serial.read() != -1);

  
}