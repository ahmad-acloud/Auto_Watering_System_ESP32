#include "certs.h"          // Get Creds. from AWS 
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "time.h"
#include "DHT.h"  

// Lib to get local time using NTP 

#include <NTPClient.h>
#include <WiFiUdp.h>

String datetime;
// Initializing sesnors pins 

const int DHTPIN = 33; // Pin connected the DHT11 sensor 
const int DHTTYPE = DHT11; // Type of DHT
const int trigPin = 16; // Triger pin
const int echoPin = 17; // Echo pin  
const int sig = 5;
const int AirValue = 1200;   //Calibration in dry soil
const int WaterValue = 450;  // Calibration in water
const float radius = 6;            //Radius of water bucket 
const float height= 12;            // Height of water bucket  
const float Pi = 3.14159; 
// Initializing functions varibles for ultrasound  & soil mositure sensor  
long duration;
float distance;

int volume;
int soilMoistureValue;

int soilmoisturepercent;

float T;  // Temp
float H;  // Humidity 
int pumpState; 

// Wifi credentials
const char *WIFI_SSID = "Your WIFI SSID";
const char *WIFI_PASSWORD = "WIFI-PASSWORD";

// Define NTP Client to get current time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//Date & Time 
String formattedDate;




// The name of the device
#define DEVICE_NAME "my-esp32-device"

// The MQTTT endpoint for the device 
#define AWS_IOT_ENDPOINT "XXXXXXXXXXXXXXXXXXXXXXXXXXX.amazonaws.com"

// The MQTT topic that this device should publish to
#define AWS_IOT_TOPIC "$aws/things/my-esp32-device/shadow/update"

// How many times we should attempt to connect to AWS
#define AWS_MAX_RECONNECT_TRIES 50

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
DHT dht(DHTPIN, DHTTYPE);      

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Only try 15 times to connect to the WiFi
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 15){
    delay(500);
    Serial.print("Connecting .. ");
    retries++;
  }

  // If we still couldn't connect to the WiFi, go to deep sleep for a minute and try again.
  if(WiFi.status() != WL_CONNECTED){
    esp_sleep_enable_timer_wakeup(1 * 60L * 1000000L);
    esp_deep_sleep_start();
  }
  else {
    Serial.print("Connected to Wifi.. ");
  }
}

void connectToAWS()
{
  // Configure WiFiClientSecure to use the AWS certificates we generated
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Try to connect to AWS and count how many times we retried.
  int retries = 0;
  Serial.print("Connecting to AWS IOT");

  while (!client.connect(DEVICE_NAME) && retries < AWS_MAX_RECONNECT_TRIES) {
    Serial.print(".");
    delay(100);
    retries++;
  }

  // Make sure that we did indeed successfully connect to the MQTT broker
  // If not we just end the function and wait for the next loop.
  if(!client.connected()){
    Serial.println(" Timeout!");
    return;
  }

  // Successfully connected to AWS!
  // And we can subscribe to topics and send messages.
  Serial.println("Connected!");
}

// Get the volume of the water tank

float getVolume() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = (pulseIn(echoPin, HIGH)); 
  distance = float(duration/29/2);         // Output values in cm

  volume = Pi* sq(radius)*(height-distance);   // Volume is in cm^3
  
   Serial.println(volume);
}

// Get the humidity and temp reading  

float getHT() {
  T = dht.readTemperature(); // return temperature in ??C
  H = dht.readHumidity();// return humidity in %
     Serial.println(T);
     Serial.println(H);
 //check whether reading was successful or not
 if(T == NAN || H == NAN){ 
    Serial.println("Reading failed.");
  }

}

// Get the moisture sensor value 
int getMoistureValue() {
  
soilMoistureValue = analogRead(32);  // Analoge pin is connected on ESP32
Serial.println(soilMoistureValue);

soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
if(soilmoisturepercent < 50)  // Lower threshold/dry soil/pump is on
{
  Serial.println("Nearly dry, Pump turning on");
  digitalWrite(sig,HIGH);  // 
  pumpState = 1;
}
else if (soilmoisturepercent > 80) //  Upper threshold/wet soil/pump is off
{
  Serial.println("Nearly wet, Pump turning off");
   digitalWrite(sig,LOW); 
  pumpState = 0;

}
}

// Send values to AWS IoT in json format 

void sendJsonToAWS()
{
  
  StaticJsonDocument<128> doc;
//  JsonObject stateObj = jsonDoc.createNestedObject("state");
//  JsonObject reportedObj = stateObj.createNestedObject("reported");
  
  // Write the temperature & humidity. Here you can use any C++ type (and you can refer to variables)
  doc["time"] = formattedDate;
  doc["temperature"] = T;              
  doc["humidity"] = H;
   
  doc["volume"] = volume;           //Water volume in bucket 
  doc["moisturePercent"] = soilmoisturepercent;     // Mositure % in soil 
  doc["pumpState"] = pumpState;          // State of water pump
  //reportedObj["wifi_strength"] = WiFi.RSSI();
  
 

  Serial.println("Publishing message to AWS...");
 serializeJson(doc, Serial);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_TOPIC, jsonBuffer);
}



void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(32,INPUT);
  pinMode(sig,OUTPUT);
  
  connectToWiFi();  
  connectToAWS();

  // Initialize a NTPClient to get current time
  timeClient.begin();
  timeClient.setTimeOffset(3600);

}

void loop() {

  getHT();
  getVolume();
  getMoistureValue();
 // Get the local time every time a reading is registered 
 while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);
  
  sendJsonToAWS();
  
  client.loop();
  delay(5000);
}
