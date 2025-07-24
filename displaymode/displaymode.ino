#include <Wire.h>
#include <RTClib.h>         // rtclib is library name
#include <ESP32Servo.h>     // esp32servo
#include <HardwareSerial.h> // esp espressif board
#include <fpm.h>            // FPM github
#include <WiFi.h>
#include <ToneESP32.h> // toneesp32

/*
clock uses pins 21 and 22
servo uses pin 15 and 0
finger print uses pins 25 TX, 32 RX
speaker red uses pin 13
button is pin 2
*/

RTC_DS3231 rtc;

Servo servoTop;
Servo servoBottom;
int servoTopPosition = -1;
int servoBottomPosition = -1;

const int checkFingerButtonPin = 2;
int checkFingerButtonState = 0;

ToneESP32 buzzer(13, 5);

HardwareSerial fserial(1);
FPM finger(&fserial);
FPMSystemParams params;

#define PRINTF_BUF_SZ 40
char printfBuf[PRINTF_BUF_SZ];

void setup()
{
  Serial.begin(57600);
  Serial.println("Fingerprint, clock, and servo");

  rtc.begin();
  Serial.println(getDateString());

  servoTop.attach(15);
  servoBottom.attach(12);

  pinMode(checkFingerButtonPin, INPUT);

  fserial.begin(57600, SERIAL_8N1, 25, 32);

  if (finger.begin())
  {
    finger.readParams(&params);
    Serial.println("Found fingerprint sensor!");
    Serial.print("Capacity: ");
    Serial.println(params.capacity);
    Serial.print("Packet length: ");
    Serial.println(FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
  }
  else
  {
    Serial.println("Did not find fingerprint sensor :(");
    while (1)
      yield();
  }
}

void loop()
{
  DateTime rtcTime = rtc.now();

  uint16_t command = 0;

  while (!Serial.available())
    ;
  command = Serial.parseInt();

  // 0 noise calibration
  //  1 noise pill forgotten
  //  2 button
  //  3 rotate top
  //  4 rotate bottom
  //  5 get date

  if (command == 0)
  { // need calibration
    Serial.println("need calibration");
    buzzer.tone(NOTE_FS7, 250);
    buzzer.tone(NOTE_C7, 250);
    buzzer.tone(NOTE_FS7, 250);
    buzzer.tone(NOTE_C7, 250);
    buzzer.tone(NOTE_FS7, 250);
    buzzer.tone(NOTE_C7, 250);
  }
  else if (command == 1)
  { // pill forgotten
    Serial.println("pill forgotten");
    buzzer.tone(NOTE_A4, 500);
    buzzer.tone(NOTE_A4, 500);
    buzzer.tone(NOTE_A4, 500);
    buzzer.tone(NOTE_F4, 350);
    buzzer.tone(NOTE_C5, 150);
    buzzer.tone(NOTE_A4, 500);
    buzzer.tone(NOTE_F4, 350);
    buzzer.tone(NOTE_C5, 150);
    buzzer.tone(NOTE_A4, 650);
    delay(500);
    buzzer.tone(NOTE_E5, 500);
    buzzer.tone(NOTE_E5, 500);
    buzzer.tone(NOTE_E5, 500);
    buzzer.tone(NOTE_F5, 350);
    buzzer.tone(NOTE_C5, 150);
    buzzer.tone(NOTE_GS4, 500);
    buzzer.tone(NOTE_F4, 350);
    buzzer.tone(NOTE_C5, 150);
    buzzer.tone(NOTE_A4, 650);
  }
  else if (command == 2)
  { // press button one
    Serial.println("press button");
    while (digitalRead(checkFingerButtonPin) != 1)
      ;
    int searchFinger = searchDatabase();
    if (searchFinger != 0)
    {
      buzzer.tone(NOTE_E7, 120);
      buzzer.tone(NOTE_E7, 120);
      delay(120);
      buzzer.tone(NOTE_E7, 120);
      delay(120);
      buzzer.tone(NOTE_C7, 120);
      buzzer.tone(NOTE_E7, 120);
      checkAndRotate(searchFinger);
    }
    else
    {
      buzzer.tone(NOTE_CS6, 62);
      buzzer.tone(NOTE_CS7, 62);
      buzzer.tone(NOTE_C7, 375);
      buzzer.tone(NOTE_GS6, 62);
      buzzer.tone(NOTE_FS6, 62);
      buzzer.tone(NOTE_GS6, 375);
    }
    while (Serial.read() != -1)
      ;
  }
  else if (command == 3)
  {
    rotateTop(false);
  }
  else if (command == 4)
  {
    rotateBottom(false);
  }
  else if (command == 5)
  {
    Serial.println(getDateString());
  }

  while (Serial.read() != -1)
    ;
}

String getDateString()
{
  DateTime rtcTime = rtc.now();
  String result = "Current Date Time: ";
  result += rtcTime.month();
  result += '/';
  result += rtcTime.day();
  result += '/';
  result += rtcTime.year();
  result += " ";
  int dow = rtcTime.dayOfTheWeek();
  switch (dow)
  {
  case 0:
    result += "Sunday";
    break;
  case 1:
    result += "Monday";
    break;
  case 2:
    result += "Tuesday";
    break;
  case 3:
    result += "Wednesday";
    break;
  case 4:
    result += "Thursday";
    break;
  case 5:
    result += "Friday";
    break;
  case 6:
    result += "Saturday";
    break;
  }
  result += " ";
  result += rtcTime.hour();
  result += ':';
  result += rtcTime.minute();
  result += ':';
  result += rtcTime.second();
  return result;
}

void rotateTop(bool changePosition)
{
  servoTop.writeMicroseconds(1600);
  delay(120);
  servoTop.writeMicroseconds(1500);
  if (changePosition)
  {
    servoTopPosition += 1;
  }
  if (servoTopPosition == 8)
  {
    servoTopPosition = 0;
  }
}
void rotateBottom(bool changePosition)
{
  servoBottom.writeMicroseconds(1600);
  delay(200);
  servoBottom.writeMicroseconds(1500);
  if (changePosition)
  {
    servoBottomPosition += 1;
  }
  if (servoBottomPosition == 8)
  {
    servoBottomPosition = 0;
  }
}

void checkAndRotate(int id)
{
  if (id == 1)
  {
    // rotate();
    /*
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    fix
    */
    // TODO
  }
  else if (id == 2)
  {
    DateTime rtcTime = rtc.now();
    int dow = rtcTime.dayOfTheWeek();
    if (dow == servoTopPosition)
    {
      if ((rtcTime.hour() > 6 && rtcTime.hour() < 10))
      {
        rotateTop(true);
      }
    }
    if (dow == servoBottomPosition)
    {
      if ((rtcTime.hour() > 18 && rtcTime.hour() < 22))
      {
        rotateBottom(true);
      }
    }
  }
}

void refillCapsules(int day)
{
  Serial.println("<body><h1>Capsule Refills</h1>");
  Serial.println("<p>Click the button to rotate the device by one day, and when you are done after returning the device to the empty slot click done</p>");
  // to deal with uncertainty have a recaliibration button and count the number of changes to the dial
}

bool enrollFinger(int16_t fid)
{
  Serial.println("<body><h1>Enrolling Finger</h1>");
  if (fid == 1)
  {
    Serial.println("<p>This is for the parents</p>");
  }
  else if (fid == 2)
  {
    Serial.println("<p>This is for the child</p>");
  }
  FPMStatus status;
  const int NUM_SNAPSHOTS = 2;

#if defined(FPM_LED_CONTROL_ENABLED)
  finger.ledOn();
#endif

  /* Take snapshots of the finger,
   * and extract the fingerprint features from each image */
  for (int i = 0; i < NUM_SNAPSHOTS; i++)
  {
    Serial.println(i == 0 ? "Place a finger" : "Place same finger again");
    Serial.println(i == 0 ? "<p>Place a finger</p>" : "<p>Place same finger again</p>");

    do
    {
#if defined(FPM_LED_CONTROL_ENABLED)
      status = finger.getImageOnly();
#else
      status = finger.getImage();
#endif

      switch (status)
      {
      case FPMStatus::OK:
        Serial.println("Image taken");
        Serial.println("<p>Image taken</p>");
        break;

      case FPMStatus::NOFINGER:
        Serial.print(".");
        break;

      default:
        /* allow retries even when an error happens */
        snprintf(printfBuf, PRINTF_BUF_SZ, "getImage(): error 0x%X", static_cast<uint16_t>(status));
        Serial.println(printfBuf);
        break;
      }

      yield();
    } while (status != FPMStatus::OK);

    status = finger.image2Tz(i + 1);

    switch (status)
    {
    case FPMStatus::OK:
      Serial.println("Image converted");
      Serial.println("<p>Image Converted</p>");
      break;

    default:
      snprintf(printfBuf, PRINTF_BUF_SZ, "image2Tz(%d): error 0x%X", i + 1, static_cast<uint16_t>(status));
      Serial.println(printfBuf);
      Serial.println("<p>Error! Please try again later</p>");
      Serial.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
      return false;
    }

    Serial.println("Remove finger");
    Serial.println("<p>Remove finger</p>");
    delay(1000);
    do
    {
#if defined(FPM_LED_CONTROL_ENABLED)
      status = finger.getImageOnly();
#else
      status = finger.getImage();
#endif
      delay(200);
    } while (status != FPMStatus::NOFINGER);
  }

#if defined(FPM_LED_CONTROL_ENABLED)
  finger.ledOff();
#endif

  /* Images have been taken and converted into features a.k.a character files.
   * Now, need to create a model/template from these features and store it. */

  status = finger.generateTemplate();
  switch (status)
  {
  case FPMStatus::OK:
    Serial.println("Template created from matching prints!");
    Serial.println("<p>Template Created from matching prints!</p>");
    break;

  case FPMStatus::ENROLLMISMATCH:
    Serial.println("The prints do not match!");
    Serial.println("<p>The prints do not match, try again later!</p>");
    Serial.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
    return false;

  default:
    snprintf(printfBuf, PRINTF_BUF_SZ, "createModel(): error 0x%X", static_cast<uint16_t>(status));
    Serial.println(printfBuf);
    Serial.println("<p>Error! Please try again later</p>");
    Serial.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
    return false;
  }

  status = finger.storeTemplate(fid);
  switch (status)
  {
  case FPMStatus::OK:
    snprintf(printfBuf, PRINTF_BUF_SZ, "Template stored at ID %d!", fid);
    Serial.println(printfBuf);
    Serial.println("<p>Fingerprint successfully stored</p>");
    break;

  case FPMStatus::BADLOCATION:
    snprintf(printfBuf, PRINTF_BUF_SZ, "Could not store in that location %d!", fid);
    Serial.println(printfBuf);
    Serial.println("<p>Error! Please try again later</p>");
    Serial.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
    return false;

  default:
    snprintf(printfBuf, PRINTF_BUF_SZ, "storeModel(): error 0x%X", static_cast<uint16_t>(status));
    Serial.println(printfBuf);
    Serial.println("<p>Error! Please try again later</p>");
    Serial.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
    return false;
  }
  Serial.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
  return true;
}

//
// FIND IF THIS FINGER IS A MATCH TO ONE IN THE DATABASE
//
int searchDatabase(void)
{
  FPMStatus status;

  /* Take a snapshot of the input finger */
  Serial.println("Place a finger.");

  do
  {
    status = finger.getImage();

    switch (status)
    {
    case FPMStatus::OK:
      Serial.println("Image taken");
      break;

    case FPMStatus::NOFINGER:
      Serial.println(".");
      break;

    default:
      /* allow retries even when an error happens */
      snprintf(printfBuf, PRINTF_BUF_SZ, "getImage(): error 0x%X", static_cast<uint16_t>(status));
      Serial.println(printfBuf);
      break;
    }

    yield();
  } while (status != FPMStatus::OK);

  /* Extract the fingerprint features */
  status = finger.image2Tz();

  switch (status)
  {
  case FPMStatus::OK:
    Serial.println("Image converted");
    break;

  default:
    snprintf(printfBuf, PRINTF_BUF_SZ, "image2Tz(): error 0x%X", static_cast<uint16_t>(status));
    Serial.println(printfBuf);
    return false;
  }

  /* Search the database for the converted print */
  uint16_t fid, score;
  status = finger.searchDatabase(&fid, &score);

  switch (status)
  {
  case FPMStatus::OK:
    snprintf(printfBuf, PRINTF_BUF_SZ, "Found a match at ID #%u with confidence %u", fid, score);
    Serial.println(printfBuf);
    Serial.println((fid == 1) ? "parents" : "child");
    break;

  case FPMStatus::NOTFOUND:
    Serial.println("Did not find a match.");
    return 0;

  default:
    snprintf(printfBuf, PRINTF_BUF_SZ, "searchDatabase(): error 0x%X", static_cast<uint16_t>(status));
    Serial.println(printfBuf);
    return 0;
  }

  /* Now wait for the finger to be removed, though not necessary.
     This was moved here after the Search operation because of the R503 sensor,
     whose searches oddly fail if they happen after the image buffer is cleared  */
  Serial.println("Remove finger.");
  delay(1000);
  do
  {
    status = finger.getImage();
    delay(200);
  } while (status != FPMStatus::NOFINGER);

  return fid;
}
