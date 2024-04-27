#include <Wire.h>    
#include <RTClib.h> // rtclib is library name
#include <ESP32Servo.h> // esp32servo
#include <HardwareSerial.h> // esp espressif board
#include <fpm.h> //FPM github
#include <WiFi.h>

/*
clock uses pins 21 and 22
servo uses pin 18
finger print uses pins 25 TX, 32 RX
*/

// Replace with your network credentials
const char* ssid = "Tumba_2G";
const char* password = "Dalal123";
// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;
//client
WiFiClient client;
// timings used to check for disconnections
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

RTC_DS3231 rtc;  

Servo servoTop;
int servoTopPosition = 0;

const int checkFingerButtonPin = 2;
int checkFingerButtonState = 0;

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

  pinMode(checkFingerButtonPin, INPUT);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

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

  checkFingerButtonState = digitalRead(checkFingerButtonPin);
  if(checkFingerButtonState == 1){
    searchDatabase();
    while (Serial.read() != -1);
  }

  client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client Connected.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            // get request handling
            if (header.indexOf("GET /finger/resetparent") >= 0) {
              Serial.println("Reseting Parent Finger");
              enrollFinger(1);
            } else if (header.indexOf("GET /finger/resetchild") >= 0) {
              Serial.println("Reseting Child Finger");
              enrollFinger(2);
            } else {
              if (header.indexOf("GET /refill/clock") >= 0) {
                Serial.println("Rotating by one slot");
                rotate();
              }


              // Web Page Heading
              client.println("<body><h1>Safe Pills</h1>");
              
              // button options  
              client.println("<h2>Reset finger prints</h2>");   
              client.println("<p><a href=\"/finger/resetparent\"><button class=\"button\">Reset Parent Finger</button></a></p>");
              client.println("<p><a href=\"/finger/resetchild\"><button class=\"button\">Reset Child Finger</button></a></p>");

              client.println("<h2>Refill</h2>");   
              client.println("<p><a href=\"/refill/clock\"><button class=\"button\">Refill (rotates one slot over) </button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void rotate(){
  servoTop.writeMicroseconds(2000);
  delay(500);
  servoTop.writeMicroseconds(1500);
  servoTopPosition += 1;
  if (servoTopPosition == 8){
    servoTopPosition = 0;
  }
}

void refillCapsules(int day){
  client.println("<body><h1>Capsule Refills</h1>");
  client.println("<p>Click the button to rotate the device by one day, and when you are done after returning the device to the empty slot click done</p>");
  //to deal with uncertainty have a recaliibration button and count the number of changes to the dial
}

bool enrollFinger(int16_t fid) 
{
    client.println("<body><h1>Enrolling Finger</h1>");
    if(fid == 1){
      client.println("<p>This is for the parents</p>");
    } else if(fid == 2){
      client.println("<p>This is for the child</p>");
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
        client.println(i == 0 ? "<p>Place a finger</p>" : "<p>Place same finger again</p>");
        
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
                    client.println("<p>Image taken</p>");
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
        }
        while (status != FPMStatus::OK);

        status = finger.image2Tz(i+1);
        
        switch (status) 
        {
            case FPMStatus::OK:
                Serial.println("Image converted");
                client.println("<p>Image Converted</p>");
                break;
                
            default:
                snprintf(printfBuf, PRINTF_BUF_SZ, "image2Tz(%d): error 0x%X", i+1, static_cast<uint16_t>(status));
                Serial.println(printfBuf);
                client.println("<p>Error! Please try again later</p>");
                client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
                return false;
        }

        Serial.println("Remove finger");
        client.println("<p>Remove finger</p>");
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
            client.println("<p>Template Created from matching prints!</p>");
            break;
            
        case FPMStatus::ENROLLMISMATCH:
            Serial.println("The prints do not match!");
            client.println("<p>The prints do not match, try again later!</p>");
            client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "createModel(): error 0x%X", static_cast<uint16_t>(status));
            Serial.println(printfBuf);
            client.println("<p>Error! Please try again later</p>");
            client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
            return false;
    }
    
    status = finger.storeTemplate(fid);
    switch (status)
    {
        case FPMStatus::OK:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Template stored at ID %d!", fid);
            Serial.println(printfBuf);
            client.println("<p>Fingerprint successfully stored</p>");
            break;
            
        case FPMStatus::BADLOCATION:
            snprintf(printfBuf, PRINTF_BUF_SZ, "Could not store in that location %d!", fid);
            Serial.println(printfBuf);
            client.println("<p>Error! Please try again later</p>");
            client.println("<p><a href=\"/\"><button class=\"button\">Return home</button></a></p>");
            return false;
            
        default:
            snprintf(printfBuf, PRINTF_BUF_SZ, "storeModel(): error 0x%X", static_cast<uint16_t>(status));
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
            Serial.println((fid == 1) ? "parents" : "child");
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
