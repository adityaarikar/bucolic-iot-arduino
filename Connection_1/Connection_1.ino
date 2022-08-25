#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "DHT.h"



AsyncWebServer server(80); // 80 is the port number

//.......................Defining all the variables starts here ........................//
const char* ssid = "Aditya";
const char* password = "123123aa";

unsigned long  presentLight = 0;
unsigned long  presentFan = 0;
unsigned long  presentWater = 0;

String ledon, ledoff, led1on, led1off;
int sprayer = D6;
const char* device = "OXY";
float temp = 0.0;
float humid = 0.0;
float airQual = 0.0;
int waterL = 00;
long lightOn = 8000;
long lightOff = 5000;
long waterOn = 5000; //5sec
long waterOff = 3000; //3sec
long fanOn = 8000;
long fanOff = 300000;
String lightOnTime = "00.0";
String lightOffTime = "00.0";
String waterOnTime = "00.0";
String waterOffTime = "00.0";
String fanOnTime = "00.0";
String fanOffTime = "00.0";
long duration;
int distance;

const char* PARAM_INPUT_1 = "waterOn";
const char* PARAM_INPUT_2 = "waterOff";


int dhtpin = D1; //DHT11
const int echoPin = D2;//ultrasonic
const int trigPin = D3;//ultrasonic
const int led = D4;//light
const int fanIn1 = D7;//Fan pin 1
const int fanIn2 = D8;//Fan pin 2
int sprayerState = HIGH;
int ledState = HIGH;

DHT dht(dhtpin, DHT11);

// Define NTP Client to get time
const long utcOffsetInSeconds = 19800;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "in.pool.ntp.org", 19800, 60000);
//.......................Defining all the variables ends here ........................//

//....................... Setup function starts here ........................//
void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)delay(500);

  Serial.print(WiFi.localIP());

  dht.begin();
  server.begin();

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(sprayer, OUTPUT);
  pinMode(led,OUTPUT);
  
  digitalWrite(sprayer,sprayerState);
  digitalWrite(led,ledState);

  //=====> Get data request
  //....................... get data api request ........................//
  //....................... sends all the data to mobile phone ........................//
  server.on("/getData", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(1024);
    json["device"] = device;
    json["ip"] = WiFi.localIP();
    json["temp"] = temp;
    json["humid"] = humid;
    json["airQual"] = airQual;
    json["waterL"] = waterL;
    json["lightOn"] = lightOn;
    json["lightOff"] = lightOff;
    json["waterOn"] = waterOn;
    json["waterOff"] = waterOff;
    json["fanOn"] = fanOn;
    json["fanOff"] = fanOff;
    json["lightOnTime"] = lightOnTime;
    json["lightOffTime"] = lightOffTime;
    json["waterOnTime"] = waterOnTime;
    json["waterOffTime"] = waterOffTime;
    json["fanOnTime"] = fanOnTime;
    json["fanOffTime"] = fanOffTime;
    serializeJson(json, *response);
    request->send(response);
  });

  //====> Light control apis
  //....................... api request to on the light ........................//
  server.on("/lighton", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(led, LOW);
    request->send_P(200, "text/plain", "OK");
    lightOnTime = timeClient.getFormattedTime();
  });

  //....................... api request to off the light ........................//
  server.on("/lightoff", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(led, HIGH);
    request->send_P(200, "text/plain", "OK");
    lightOffTime = timeClient.getFormattedTime();
  });

  server.on("/lightTimeSetting", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("lightOn") && request->hasParam("lightOff")) {
      lightOn = request->getParam("lightOn")->value().toInt();
      lightOff = request->getParam("lightOff")->value().toInt();
    }

    request->send_P(200, "text/plain", "OK");
  });

  //====> Fan control apis
  //....................... api request to on the fan ........................//
  server.on("/fanon", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(fanIn1, LOW);
    digitalWrite(fanIn2, HIGH);
    request->send_P(200, "text/plain", "OK");
  });

  //....................... api request to off the fan ........................//
  server.on("/fanoff", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(fanIn1, LOW);
    digitalWrite(fanIn2, LOW);
    request->send_P(200, "text/plain", "OK");
  });

  server.on("/fanTimeSetting", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("fanOn") && request->hasParam("fanOff")) {
      fanOn = request->getParam("fanOn")->value().toInt();
      fanOff = request->getParam("fanOff")->value().toInt();
    }
    request->send_P(200, "text/plain", "OK");
  });

  //  ====> spray control apis
  //  ....................... api request to on the spray ........................//
  server.on("/sprayon", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(sprayer, LOW);
    waterOnTime = timeClient.getFormattedTime();
    request->send_P(200, "text/plain", "OK");
  });

  //....................... api request to off the spray ........................//
  server.on("/sprayoff", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(sprayer, HIGH);
    waterOffTime = timeClient.getFormattedTime();
    request->send_P(200, "text/plain", "OK");
  });

  server.on("/sprayTimeSetting", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("waterOn") && request->hasParam("waterOff")) {
      waterOn = request->getParam("waterOn")->value().toInt();
      waterOff = request->getParam("waterOff")->value().toInt();
    }
    request->send_P(200, "text/plain", "OK");
  });  

  timeClient.begin();
}

void loop()
{
  timeClient.update();
//  unsigned long currentMillis = millis();

  if(sprayerState == HIGH){
    if((millis() - presentWater) >= waterOn){
      sprayerState = LOW;
      presentWater = millis();
      waterOnTime = timeClient.getFormattedTime();
    }
  }else{
    if((millis() - presentWater) >= waterOff){
      sprayerState = HIGH;
      presentWater = millis();
      waterOffTime = timeClient.getFormattedTime();
    }
  }
  digitalWrite(sprayer,sprayerState);

  if(ledState == HIGH){
    if((millis() - presentLight) >= lightOn){
      ledState = LOW;
      presentLight = millis();
      lightOnTime = timeClient.getFormattedTime();
    }
  }else{
    if((millis() - presentLight) >= lightOff){
      ledState = HIGH;
      presentLight = millis();
      lightOffTime = timeClient.getFormattedTime();
    }
  }
  digitalWrite(led,ledState);

  humid = dht.readHumidity();
  temp = dht.readTemperature();

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.println(distance);

  delay(200);
}
