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
  const char txt[6][15] = { "year [4-digit]", "month [1~12]", "day [1~31]",
                            "hours [0~23]", "minutes [0~59]", "seconds [0~59]"};
  String str = "";
  long newDate[6];

  while (Serial.available()) {
    Serial.read();  // clear serial buffer
  }

  for (int i = 0; i < 6; i++) {

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
  rtc.adjust(DateTime(newDate[0], newDate[1], newDate[2], newDate[3], newDate[4], newDate[5]));
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
      DateTime current = rtc.now();
      Serial.print("Current Date Time: ");
      Serial.print(current.year(), DEC);
      Serial.print('/');
      Serial.print(current.month(), DEC);
      Serial.print('/');
      Serial.print(current.day(), DEC);
      Serial.print(" (");
      Serial.print(current.dayOfTheWeek());
      Serial.print(") ");
      Serial.print(current.hour(), DEC);
      Serial.print(':');
      Serial.print(current.minute(), DEC);
      Serial.print(':');
      Serial.println(current.second(), DEC);
    }
    else if (input == 'u') updateRTC();  // update RTC time
  }
}