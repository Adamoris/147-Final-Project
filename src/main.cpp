#include <WiFi.h>
#include <HttpClient.h>
#include <Adafruit_Sensor.h>
#include <SparkFunLSM6DSO.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <DHT.h>
#include <DHT_U.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <spo2_algorithm.h>
#include <string>
#include <sstream>
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DHT11_PIN 13
#define PIEZO_IN 36
#define PIEZO_OUT 37
#define DHTTYPE DHT11

DHT dht(DHT11_PIN, DHT11);
// This example downloads the URL "http://arduino.cc/"

LSM6DSO myIMU; //Default constructor is I2C, addr 0x6B
MAX30105 particleSensor;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

float stepThreshold = 2.2;
float resetThreshold = 1.1;
float cooldown = 700;
long timer = 0;
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
}

void loop() {
  reading = analogRead(PIEZO_IN);
  float h = dht.readHumidity();
  delay(10);
  float t = dht.readTemperature();

  long irValue = particleSensor.getIR();

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
  }

  if (timer>= 50) {
    timer = 0;
    Serial.println("=========================");

    Serial.print("Piezoelectric Reading: ");
    Serial.println(reading);

    Serial.print('\n');
    Serial.print("Humidity = ");
    Serial.print(h);
    Serial.print("%  ");
    Serial.print("Temperature = ");
    Serial.print(t);
    Serial.print("°C \n");

    Serial.print('\n');
    if (irValue < 50000) {
      Serial.println("Oximeter Reading: no finger detected");
    } else {
      Serial.print("Oximeter Reading: ");
      Serial.print("IR = ");
      Serial.print(irValue);
      Serial.print(", BPM = ");
      Serial.print(beatsPerMinute);
      Serial.print(", Avg BPM = ");
      Serial.println(beatAvg);
    }

    Serial.print("\nAccelerometer:\n");
    Serial.print(" X = ");
    Serial.print(myIMU.readFloatAccelX(), 3);
    Serial.print(" Y = ");
    Serial.print(myIMU.readFloatAccelY(), 3);
    Serial.print(" Z = ");
    Serial.println(myIMU.readFloatAccelZ(), 3);

    Serial.print("\nGyroscope:\n");
    Serial.print(" X = ");
    Serial.print(myIMU.readFloatGyroX(), 3);
    Serial.print(" Y = ");
    Serial.print(myIMU.readFloatGyroY(), 3);
    Serial.print(" Z = ");
    Serial.println(myIMU.readFloatGyroZ(), 3);
    
    Serial.println("=========================");
  }
  timer++;
}