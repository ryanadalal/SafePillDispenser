// comments designate arudion library manager names
#include <Wire.h>
#include <RTClib.h>         // rtclib
#include <ESP32Servo.h>     // esp32servo
#include <HardwareSerial.h> // esp espressif board
#include <fpm.h>            // FPM github
#include <WiFi.h>
#include <ToneESP32.h> // toneesp32

/*
In assembling the physical device:
- clock uses pins 21 and 22
- servo uses pin 15 and 0
- finger print uses pins 25 TX, 32 RX
- speaker red uses pin 13
- button is pin 2
*/

#define PRINTF_BUFFER_SIZE 40

// network credentials
const char *ssid = "FILL_IN_NETWORK_NAME";
const char *password = "PASSWORD";

// Set web server port number to 80
WiFiServer server(80);
String header; // for storing HTTP requests
WiFiClient client;

// timings used to check for disconnections
unsigned long tPresent = millis();
unsigned long tPast = 0;
const long maxTime = 4000;

RTC_DS3231 rtc;

Servo servoTop;
Servo servoBottom;
// rotational position of the servers in day of the week
int servoTopPosition = -1;
int servoBottomPosition = -1;

const int checkFingerButtonPin = 2;
int checkFingerButtonState = 0;

// set up the buzzer
ToneESP32 buzzer(13, 5);

HardwareSerial fserial(1);
FPM finger(&fserial);
FPMSystemParams params;

char printfBuf[PRINTF_BUFFER_SIZE];

void setup()
{
  Serial.begin(57600);
  Serial.println("Safe Pill Dispenser");

  // initialize the clock
  rtc.begin();
  Serial.println(getDateString());

  // assign servo motors
  servoTop.attach(15);
  servoBottom.attach(12);

  // prepare for finger print input
  pinMode(checkFingerButtonPin, INPUT);

  // connect wifi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // waits for wifi to connect
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // prints the devices ip address
  Serial.println("");
  Serial.println("wifi connection success");
  Serial.println("ip address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // initiates hardware serial for fingerprint reader
  fserial.begin(57600, SERIAL_8N1, 25, 32);

  // attempt to initiate the finger print sensor
  if (finger.begin())
  {

    finger.readParams(&params);
    Serial.println("found finger print sensor with capacity ");
    Serial.println(params.capacity);
    Serial.println(" and packet length ") ;
    Serial.println(FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
  }
  else
  {
    Serial.println("failed to find finger print sensor");
    exit(0);
  }
}

void loop()
{
  // checks the current time
  DateTime rtcTime = rtc.now();

  // plays error noises if the positions are incorrect
  if (servoBottomPosition == -1 || servoTopPosition == -1)
  {
    buzzer.tone(NOTE_FS7, 250);
    buzzer.tone(NOTE_C7, 250);
  }
  else
  {
    // checks if the user wants to verify a finger print
    checkFingerButtonState = digitalRead(checkFingerButtonPin);
    if (checkFingerButtonState == 1)
    {
      // checks if the fingerprint is a valid print
      int searchFinger = searchDatabase();
      if (searchFinger != 0)
      {
        // plays a positive tone
        buzzer.tone(NOTE_E7, 120);
        buzzer.tone(NOTE_E7, 120);
        delay(120);
        buzzer.tone(NOTE_E7, 120);
        delay(120);
        buzzer.tone(NOTE_C7, 120);
        buzzer.tone(NOTE_E7, 120);
        // rotates the device if the timing is correct
        checkAndRotate(searchFinger);
      }
      else
      {
        // plays error tone
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
  }

  // check for a web client connetion
  client = server.available();
  if (client)
  {
    // variables used for time out checks
    tPresent = millis();
    tPast = tPresent;
    Serial.println("new client connection");
    // initializes incoming client request storage
    String currentLine = "";
    // ensures the client has not been lost
    while (client.connected() && tPresent - tPast <= maxTime)
    {
      tPresent = millis();
      // checks for incoming requests prints and then handles them
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n')
        {
          // send a response if the client has finished sending requests
          if (currentLine.length() == 0)
          {
            // return the base html for the webpage
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>");
            client.println("html { display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { margin: 2px; background-color: #FF0000; padding: 15px 30px; color: white; }");
            client.println(".button2 {background-color: #0000AA;}");
            client.println("</style>");
            client.println("</head>");

            // handler for a new request from the client

            // on user says calibratoin complete
            if (header.indexOf("GET /calibrate/done") >= 0)
            {
              Serial.println("Done Calibrating");
              servoTopPosition = 0;
              servoBottomPosition = 0;
            }
            // user currently calibrating top or bottom
            if (servoTopPosition == -1 || servoBottomPosition == -1)
            {
              // on user adjusting the rotation of the top or bottom
              if (header.indexOf("GET /calibrate/top") >= 0)
              {
                Serial.println("Rotating top by one slot");
                rotateTop(false);
              }
              if (header.indexOf("GET /calibrate/bottom") >= 0)
              {
                Serial.println("Rotating bottom by one slot");
                rotateBottom(false);
              }

              // begin transmitting the rest of the html page for recalbiration
              client.println("<body>");
              client.println("<h1>Recalibration Needed!</h1>");

              // outputs the time to the user
              client.print("<p>");
              client.print(getDateString());
              client.println("</p>");

              client.println("<h2>Please rotate each capsule until the blank day is visible, when you are finished select -Done Calibrating-</h2>");

              // buttons for the user to press for calibration
              client.println("<p><a href=\"/calibrate/top\"><button class=\"button\">Rotate Top (rotates one slot over) </button></a></p>");
              client.println("<p><a href=\"/calibrate/bottom\"><button class=\"button\">Rotate Bottom (rotates one slot over) </button></a></p>");
              client.println("<p><a href=\"/calibrate/done\"><button class=\"button\">Done Calibrating</button></a></p>");
            }
            // on user request to reset the parent fingerprint
            else if (header.indexOf("GET /finger/resetparent") >= 0)
            {
              Serial.println("Reseting Parent Finger");
              enrollFinger(1);
            }
            // on user request to reset the child fingerprint
            else if (header.indexOf("GET /finger/resetchild") >= 0)
            {
              Serial.println("Reseting Child Finger");
              enrollFinger(2);
            }
            else
            {
              // on user refilling the top - manually rotating
              if (header.indexOf("GET /refill/top") >= 0)
              {
                Serial.println("Rotating top by one slot");
                rotateTop(true);
              }
              // on user refilling the bottom - manually rotating
              if (header.indexOf("GET /refill/bottom") >= 0)
              {
                Serial.println("Rotating bottom by one slot");
                rotateBottom(true);
              }

              // standard home page
              client.println("<body>");
              client.println("<h1>Safe Pills</h1>");

              // time
              client.print("<p>");
              client.print(getDateString());
              client.println("</p>");

              // button options
              client.println("<h2>Reset finger prints</h2>");
              client.println("<p><a href=\"/finger/resetparent\"><button class=\"button\">Reset Parent Finger</button></a></p>");
              client.println("<p><a href=\"/finger/resetchild\"><button class=\"button\">Reset Child Finger</button></a></p>");

              client.println("<h2>Refill</h2>");
              client.println("<p><a href=\"/refill/top\"><button class=\"button\">Refill Top (rotates one slot over) </button></a></p>");
              client.println("<p><a href=\"/refill/bottom\"><button class=\"button\">Refill Bottom (rotates one slot over) </button></a></p>");
            }
            client.println("</body></html>");

            // end the response
            client.println();

            break;
          }
          else
          {
            // reset
            currentLine = "";
          }
        }
        // add any non returns to the end of the current line
        else if (c != '\r')
        {
          currentLine += c;
        }
      }
    }
    // clear the header and close the connection
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  else
  {
    // play a tune if the pill was forgotten
    DateTime rtcTime = rtc.now();
    int dow = rtcTime.dayOfTheWeek();
    if (dow == servoTopPosition && rtcTime.hour() > 10 || dow == servoBottomPosition && rtcTime.hour() > 22)
    {
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
  }
}

/*
 * gets the time from the real time clock module
 * returns - date and time in a human readable form
 */
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

// rotations for the top and bottom servos
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

// checks if device should rotate
void checkAndRotate(int id)
{
  // allows the parent to override and rotate if the child misses a pill
  if (id == 1)
  {
    DateTime rtcTime = rtc.now();
    int dow = rtcTime.dayOfTheWeek();
    if (dow == servoTopPosition)
    {
      if ((rtcTime.hour() > 3 && rtcTime.hour() < 14))
      {
        rotateTop(true);
      }
    }
    if (dow == servoBottomPosition)
    {
      if ((rtcTime.hour() > 14 || rtcTime.hour() < 3))
      {
        rotateBottom(true);
      }
    }
  }
  // rotates if the timing is right
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

// prints the html for refilling the capsules
void refillCapsules(int day)
{
  client.println("<body>");
  client.println("<h1>Capsule Refills</h1>");
  client.println("<p>Click the button to rotate the device by one day, and when you are done after returning the device to the empty slot click done</p>");
}

// enrolls new finger prints
bool enrollFinger(int16_t fid)
{
  client.println("<body>");
  client.println("<h1>Enrolling Finger</h1>");
  if (fid == 1)
  {
    client.println("<p>This is for the parents</p>");
  }
  else if (fid == 2)
  {
    client.println("<p>This is for the child</p>");
  }

  // credit: finger print module code uses example code from https://github.com/brianrho/FPM
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
    client.println(i == 0 ? "<p>Place a finger</p>" : "<p>Place same finger again</p>");

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
        client.println("<p>Image taken</p>");
        break;

      case FPMStatus::NOFINGER:
        Serial.print(".");
        break;

      default:
        /* allow retries even when an error happens */
        snprintf(printfBuf, PRINTF_BUFFER_SIZE, "getImage(): error 0x%X", static_cast<uint16_t>(status));
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
      client.println("<p>Image Converted</p>");
      break;

    default:
      snprintf(printfBuf, PRINTF_BUFFER_SIZE, "image2Tz(%d): error 0x%X", i + 1, static_cast<uint16_t>(status));
      Serial.println(printfBuf);
      client.println("<p>Error! Please try again later</p>");
      client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
      return false;
    }

    Serial.println("Remove finger");
    client.println("<p>Remove finger</p>");
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
    client.println("<p>Template Created from matching prints!</p>");
    break;

  case FPMStatus::ENROLLMISMATCH:
    Serial.println("The prints do not match!");
    client.println("<p>The prints do not match, try again later!</p>");
    client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
    return false;

  default:
    snprintf(printfBuf, PRINTF_BUFFER_SIZE, "createModel(): error 0x%X", static_cast<uint16_t>(status));
    Serial.println(printfBuf);
    client.println("<p>Error! Please try again later</p>");
    client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
    return false;
  }

  status = finger.storeTemplate(fid);
  switch (status)
  {
  case FPMStatus::OK:
    snprintf(printfBuf, PRINTF_BUFFER_SIZE, "Template stored at ID %d!", fid);
    Serial.println(printfBuf);
    client.println("<p>Fingerprint successfully stored</p>");
    break;

  case FPMStatus::BADLOCATION:
    snprintf(printfBuf, PRINTF_BUFFER_SIZE, "Could not store in that location %d!", fid);
    Serial.println(printfBuf);
    client.println("<p>Error! Please try again later</p>");
    client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
    return false;

  default:
    snprintf(printfBuf, PRINTF_BUFFER_SIZE, "storeModel(): error 0x%X", static_cast<uint16_t>(status));
    Serial.println(printfBuf);
    client.println("<p>Error! Please try again later</p>");
    client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
    return false;
  }
  client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
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
      snprintf(printfBuf, PRINTF_BUFFER_SIZE, "getImage(): error 0x%X", static_cast<uint16_t>(status));
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
    snprintf(printfBuf, PRINTF_BUFFER_SIZE, "image2Tz(): error 0x%X", static_cast<uint16_t>(status));
    Serial.println(printfBuf);
    return false;
  }

  /* Search the database for the converted print */
  uint16_t fid, score;
  status = finger.searchDatabase(&fid, &score);

  switch (status)
  {
  case FPMStatus::OK:
    snprintf(printfBuf, PRINTF_BUFFER_SIZE, "Found a match at ID #%u with confidence %u", fid, score);
    Serial.println(printfBuf);
    Serial.println((fid == 1) ? "parents" : "child");
    break;

  case FPMStatus::NOTFOUND:
    Serial.println("Did not find a match.");
    return 0;

  default:
    snprintf(printfBuf, PRINTF_BUFFER_SIZE, "searchDatabase(): error 0x%X", static_cast<uint16_t>(status));
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
// end of credit