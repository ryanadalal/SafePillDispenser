#include <Wire.h>    
#include <RTClib.h>                 // for RTC rtclib is library name

RTC_DS3231 rtc;     
//pins 21 and 22
/*
   function to update RTC time using user input
*/
void updateRTC()
{

  // ask user to enter new date and time
  const char txt[3][15] = {"hours [0~23]", "minutes [0~59]", "seconds [0~59]"};
  String str = "";
  long newDate[3];

  while (Serial.available()) {
    Serial.read();  // clear serial buffer
  }

  for (int i = 0; i < 3; i++) {

    Serial.print("Enter ");
    Serial.print(txt[i]);
    Serial.print(": ");

    while (!Serial.available()) {
      ; // wait for user input
    }

    str = Serial.readString();  // read user input
    newDate[i] = str.toInt();   // convert user input to number and save to array

    Serial.println(newDate[i]); // show user input
  }

  // update RTC
  rtc.adjust(DateTime(1,1,1,newDate[0], newDate[1], newDate[2]));
  Serial.println("RTC Updated!");
}

void setup()
{
  Serial.begin(115200); // initialize serial

  rtc.begin();       // initialize rtc
}

void loop()
{
  if(Serial.available()){
    char input = Serial.read();
    if(input == 't'){
      DateTime rtcTime = rtc.now();
      Serial.print(rtcTime.hour()); 
      Serial.print( " : ");
      Serial.print( rtcTime.minute());
      Serial.print( " : ");
      Serial.println( rtcTime.second());
    }
    else if (input == 'u') updateRTC();  // update RTC time
  }
}