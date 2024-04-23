#include <Wire.h>    
#include <RTClib.h> // rtclib is library name
#include <ESP32Servo.h> // esp32servo
#include <HardwareSerial.h> // esp espressif board
#include <fpm.h> //FPM github

/*
clock uses pins 21 and 22
servo uses pin 18
finger print uses pins 25 TX, 32 RX
*/

RTC_DS3231 rtc;  

Servo servoTop;

HardwareSerial fserial(1);
FPM finger(&fserial);
FPMSystemParams params;

#define PRINTF_BUF_SZ   40
char printfBuf[PRINTF_BUF_SZ];

void setup()
{
  Serial.begin(57600);
  Serial.println("Fingerprint, clock, and servo");

  rtc.begin();   
  servoTop.attach(18);    

  fserial.begin(57600, SERIAL_8N1, 25, 32);

  if (finger.begin()) {
      finger.readParams(&params);
      Serial.println("Found fingerprint sensor!");
      Serial.print("Capacity: "); Serial.println(params.capacity);
      Serial.print("Packet length: "); Serial.println(FPM::packetLengths[static_cast<uint8_t>(params.packetLen)]);
  } 
  else {
      Serial.println("Did not find fingerprint sensor :(");
      while (1) yield();
  }
}

void loop()
{
  DateTime rtcTime = rtc.now();
  Serial.print(rtcTime.hour()); 
  Serial.print( " : ");
  Serial.print( rtcTime.minute());
  Serial.print( " : ");
  Serial.println( rtcTime.second());

  //servoTop.write(180);

  Serial.println("Please type in the ID # (from 1 or 2) you want to save this finger as..."); 
  uint16_t fid = 0;
  while (fid != 1 && fid != 2) {
    while (! Serial.available());
    fid = Serial.parseInt();
  }
  enrollFinger(fid); 
  while (Serial.read() != -1);  // clears buffer

  searchDatabase();
  while (Serial.read() != -1); // clears buffer
}

bool enrollFinger(int16_t fid) 
{
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
        
        do {
#if defined(FPM_LED_CONTROL_ENABLED)
            status = finger.getImageOnly();
#else
            status = finger.getImage();
#endif
            
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
        }
        while (status != FPMStatus::OK);

        status = finger.image2Tz(i+1);
        
        switch (status) 
        {
            case FPMStatus::OK:
                Serial.println("Image converted");
                break;
                
            default:
                snprintf(printfBuf, PRINTF_BUF_SZ, "image2Tz(%d): error 0x%X", i+1, static_cast<uint16_t>(status));
                Serial.println(printfBuf);
                return false;
        }

        Serial.println("Remove finger");
        delay(1000);
        do {
#if defined(FPM_LED_CONTROL_ENABLED)
            status = finger.getImageOnly();
#else
            status = finger.getImage();
#endif
            delay(200);
        }
        while (status != FPMStatus::NOFINGER);
        
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
            break;
            
        case FPMStatus::ENROLLMISMATCH:
            Serial.println("The prints do not match!");
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "createModel(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    status = finger.storeTemplate(fid);
    switch (status)
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Template stored at ID %d!", fid);
            Serial.println(printfBuf);
            break;
            
        case FPMStatus::BADLOCATION:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Could not store in that location %d!", fid);
            Serial.println(printfBuf);
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "storeModel(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    return true;
}

//
// FIND IF THIS FINGER IS A MATCH TO ONE IN THE DATABASE
//
bool searchDatabase(void) 
{
    FPMStatus status;
    
    /* Take a snapshot of the input finger */
    Serial.println("Place a finger.");
    
    do {
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
    }
    while (status != FPMStatus::OK);
    
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
            break;
            
        case FPMStatus::NOTFOUND:
            Serial.println("Did not find a match.");
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "searchDatabase(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            return false;
    }
    
    /* Now wait for the finger to be removed, though not necessary. 
       This was moved here after the Search operation because of the R503 sensor, 
       whose searches oddly fail if they happen after the image buffer is cleared  */
    Serial.println("Remove finger.");
    delay(1000);
    do {
        status = finger.getImage();
        delay(200);
    }
    while (status != FPMStatus::NOFINGER);
    
    return true;
}
