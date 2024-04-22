#include <ESP32Servo.h>
//esp32servo kevin harrington
Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position

void setup() {
  myservo.attach(18);  // attaches the servo on pin 9 to the servo object
}

void loop() {
  //myservo.write(180);
  delay(3000);
  //myservo.write(0);
  delay(3000);
  
}