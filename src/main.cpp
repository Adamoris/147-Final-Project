#include <WiFi.h>
#include <HttpClient.h>
#include <Adafruit_Sensor.h>
#include <SparkFunLSM6DSO.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <DHT.h>
#include <DHT_U.h>
#include <string>
#include <sstream>
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DHT11_PIN 12
#define PIEZO_IN 36
#define PIEZO_OUT 39
#define DHTTYPE DHT11
DHT dht(DHT11_PIN, DHT11);
// This example downloads the URL "http://arduino.cc/"

LSM6DSO myIMU; //Default constructor is I2C, addr 0x6B
float stepThreshold = 2.2;
float resetThreshold = 1.1;
float cooldown = 700;
float timer = 0;
bool stepping = false;
int steps = 0;
BLECharacteristic *pCharacteristic;
int reading = 0;
int ledState = LOW;

class MyCallbacks: public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *pCharacteristic) {
     std::string value = pCharacteristic->getValue();
 
     if (value.length() > 0) {
       Serial.println("*********");
       Serial.print("New value: ");
       for (int i = 0; i < value.length(); i++)
         Serial.print(value[i]);
 
       Serial.println();
       Serial.println("*********");
       if (!value.compare("/LED on")) {
         //digitalWrite(LED_TOGGLE, HIGH);
         Serial.println("LED on!");
       } else if (!value.compare("/LED off")) {
         //digitalWrite(LED_TOGGLE, LOW);
         Serial.println("LED off!");
       }
     }
   }
};

char ssid[] = "Andrew iPhone";    // your network SSID (name) 
char pass[] = "LilygoTTGO"; // your network password (use for WPA, or use as key for WEP)

// Name of the server we want to connect to
// const char kHostname[] = "worldtimeapi.org";
const char kHostname[] = "3.101.126.187";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
const uint16_t kHttpPort = 5000;
// const char kPath[] = "/api/timezone/Europe/London.txt";
const char kPath[] = "/?var=10";

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

void setup() {
  Serial.begin(115200);
  pinMode(PIEZO_OUT, OUTPUT);
  /*
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // We start by connecting to a WiFi network
  delay(1000);
 
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
*/
  dht.begin();
  delay(1000);
}

void loop() {
  reading = analogRead(PIEZO_IN);

  if (reading >= 100) {
    ledState = !ledState;
    digitalWrite(PIEZO_OUT, ledState);
  }
/*
  int err = 0;
  
  WiFiClient c;
  HttpClient http(c);

  float h = dht.readHumidity();
  delay(10);
  float t = dht.readTemperature();

  Serial.print('\n');
  Serial.print("Humidity = ");
  Serial.print(h);
  Serial.print("%  ");
  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.print("°C \n");
  // delay(1000);

  std::ostringstream oss;
  oss << "/?temp=" << t << "&humid=" << h;
  std::string var = oss.str();

  char* sensorPath = const_cast<char*>(var.c_str());

  //char kPath[] = "/?var=10&temp=69.0&humid=83.2";
  err = http.get(kHostname, kHttpPort, sensorPath);
  if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");
      
        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ( (http.connected() || http.available()) &&
               ((millis() - timeoutStart) < kNetworkTimeout) )
        {
            if (http.available())
            {
                c = http.read();
                // Print out this character
                Serial.print(c);
               
                bodyLen--;
                // We read something, reset the timeout counter
                timeoutStart = millis();
            }
            else
            {
                // We haven't got any data, so let's pause to allow some to
                // arrive
                delay(kNetworkDelay);
            }
        }
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else
    {    
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
*/
  // Redo the loop after delay
  delay(100);
}