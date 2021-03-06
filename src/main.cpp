#include <WiFi.h>
#include <HttpClient.h>
#include <Adafruit_Sensor.h>
#include <SparkFunLSM6DSO.h>
#include <Wire.h>
#include <DHT.h>
#include <DHT_U.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <spo2_algorithm.h>
#include <Pulse.h>
#include <string>
#include <sstream>
#include <TFT_eSPI.h>
#define DHT11_PIN 13
#define PIEZO_IN 36
#define PIEZO_OUT 37
#define DEHUMIDIFIER_OUT 2
#define DHTTYPE DHT11

// Initialized Variables
DHT dht(DHT11_PIN, DHT11);
LSM6DSO myIMU; //Default constructor is I2C, addr 0x6B
MAX30105 particleSensor;
Pulse pulseIR;
Pulse pulseRed;
float SPO2;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;
int reading = 0;
long timer = 0;

char ssid[] = "Andrew iPhone";  // your network SSID (name) 
char pass[] = "LilygoTTGO"; // your network password (use for WPA, or use as key for WEP)

// Name of the server we want to connect to
const char kHostname[] = "54.215.98.118";
const uint16_t kHttpPort = 5000;

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

TFT_eSPI tft = TFT_eSPI();

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

  pinMode(PIEZO_OUT, OUTPUT);
  pinMode(DEHUMIDIFIER_OUT, OUTPUT);
  delay(500); 

  Wire.begin();
  delay(10);

  if( myIMU.begin() )
    Serial.println("IMU Ready.");
  else { 
    Serial.println("Could not connect to IMU.");
    Serial.println("Freezing");
  }

  if( myIMU.initialize(BASIC_SETTINGS) )

  if (particleSensor.begin(Wire, I2C_SPEED_FAST) == false) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  dht.begin();
  delay(1000);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
    long now = millis();   //start time of this cycle
    int err =0;
    
    WiFiClient c;
    HttpClient http(c);

    // Get sensor readings
    reading = analogRead(PIEZO_IN);
    float h = dht.readHumidity();
    if (h >= 60) {
      digitalWrite(DEHUMIDIFIER_OUT, HIGH);
    } else {
      digitalWrite(DEHUMIDIFIER_OUT, LOW);
    }
    delay(10);
    float t = dht.readTemperature();

    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();

    if (checkForBeat(irValue) == true)
    {
      //We sensed a beat!
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20)
      {
        rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable

        //Take average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }

      // SPO2 COMPUTATION
      int16_t IR_signal, Red_signal;

      IR_signal =  pulseIR.ma_filter(pulseIR.dc_filter(irValue)) ;
      Red_signal = pulseRed.ma_filter(pulseRed.dc_filter(redValue));

      int redDC = pulseRed.avgDC();
      int irDC = pulseIR.avgDC();
      float reading1 = (float)redValue / (float)redDC;
      float reading2 = (float)irValue / (float)irDC;
      
      // plug filtered AC/DC IR and Red readings into arterial SPO2 formula and normalize
      SPO2 = min(reading1, reading2) / max(reading1, reading2) * 98.98989;
      Serial.print("Red: ");
      Serial.println(reading1, 5);
      Serial.print("IR: ");
      Serial.println(reading2, 5);
      
      Serial.print("SPO2 ==> ");
      Serial.println(SPO2, 5);
    }

    float acc_X = myIMU.readFloatAccelX();
    float acc_Y = myIMU.readFloatAccelY();
    float acc_Z = myIMU.readFloatAccelZ();

    float gyro_X = myIMU.readFloatGyroX();
    float gyro_Y = myIMU.readFloatGyroY();
    float gyro_Z = myIMU.readFloatGyroZ();

    std::ostringstream oss;
    oss << "/?temp=" << t << "&humid=" << h << "&piezo=" << reading << "&beatAvg=" << beatAvg << "&spo2=" << SPO2 << "&acc_x=" << acc_X << "&acc_y=" << acc_Y << "&acc_z=" << acc_Z << "&gyro_x=" << gyro_X << "&gyro_y=" << gyro_Y << "&gyro_z=" << gyro_Z;
    std::string var = oss.str();

    char* sensorPath = const_cast<char*>(var.c_str());

    // Display readings
    if (timer>= 40) {
      //Send data to server
      err = http.get(kHostname, kHttpPort, sensorPath);
      int debug = 0;
      if (err == 0 && debug)
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
            // Serial.print("Content length is: ");
            // Serial.println(bodyLen);
            // Serial.println();
            // Serial.println("Body returned follows:");
          
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
      else if (debug)
      {
        Serial.print("Connect failed: ");
        Serial.println(err);
      }
      http.stop();

      // Print info on Serial Monitor      
      timer = 0;
      Serial.println("\n=========================");
      Serial.print("SPO2: ");
      Serial.println(SPO2);

      Serial.print("Piezoelectric Reading: ");
      Serial.println(reading);

      Serial.print('\n');
      Serial.print("Humidity = ");
      Serial.print(h);
      Serial.print("%  ");
      Serial.print("Temperature = ");
      Serial.print(t);
      Serial.print("??C \n");

      Serial.print('\n');
      if (irValue < 50000) {
        Serial.println("Oximeter Reading: no finger detected");
        tft.setTextDatum(BC_DATUM);
        tft.setTextColor(TFT_RED);  tft.setTextSize(3);
        tft.fillScreen(TFT_BLACK);
        tft.drawString("No Pulse", (int)(tft.width()/2), (int)(tft.height()/2), 2);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("Detected!", (int)(tft.width()/2), (int)(tft.height()/2), 2);
      } else {
        Serial.print("Oximeter Reading: ");
        Serial.print("IR = ");
        Serial.print(irValue);
        Serial.print(", Red = ");
        Serial.print(redValue);
        Serial.print(", BPM = ");
        Serial.print(beatsPerMinute);
        Serial.print(", Avg BPM = ");
        Serial.println(beatAvg);

        tft.setTextDatum(BC_DATUM);
        tft.setTextColor(TFT_RED);  tft.setTextSize(3);
        tft.fillScreen(TFT_BLACK);
        tft.drawNumber(beatAvg, (int)(tft.width()/2), (int)(tft.height()/2), 2);
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(TFT_WHITE);  tft.setTextSize(3);
        tft.drawString("Heartrate", (int)(tft.width()/2), (int)(tft.height()/2), 2);
      }

      Serial.print("\nAccelerometer:\n");
      Serial.print(" X = ");
      Serial.print(acc_X, 3);
      Serial.print(" Y = ");
      Serial.print(acc_Y, 3);
      Serial.print(" Z = ");
      Serial.println(acc_Z, 3);

      Serial.print("\nGyroscope:\n");
      Serial.print(" X = ");
      Serial.print(gyro_X, 3);
      Serial.print(" Y = ");
      Serial.print(gyro_Y, 3);
      Serial.print(" Z = ");
      Serial.println(gyro_Z, 3);
      
      Serial.println("=========================");
    }
    timer++;
}