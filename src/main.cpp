#include <WiFi.h>
#include <HttpClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <string>
#include <sstream>

#define DHT11_PIN 23
#define DHTTYPE DHT11
DHT dht(DHT11_PIN, DHT11);
// This example downloads the URL "http://arduino.cc/"

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

  dht.begin();
  delay(1000);
}

void loop() {

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

  // Redo the loop after delay
  delay(1000);
}