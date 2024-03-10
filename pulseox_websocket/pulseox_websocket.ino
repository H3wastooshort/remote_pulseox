#include <Wire.h>
#include <MAX30105.h>
#include "heartRate.h"
//#include "spo2_algorithm.h"
#include <ArduinoOTA.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <FS.h>
#include <LittleFS.h>

#include "secrets.h"

constexpr uint8_t ws_sample_rate = 75;  //period in ms

MAX30105 particleSensor;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  Wire.pins(0, 2);  //yes this is depreciated but its the only way to make it work with the MAX30102 lib

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))  //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1)
      ;
  }

  byte ledBrightness = 0x2F;  //Options: 0=Off to 255=50mA
  byte sampleAverage = 32;    //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 3;           //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 800;       //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 215;       //Options: 69, 118, 215, 411
  int adcRange = 16384;       //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);  //Configure sensor with these settings

  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  uint16_t tenths_searching = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(F("."));
    tenths_searching++;
    if (tenths_searching > 300) {  //after 30 seconds
      //WiFi.mode(WIFI_AP);
      //WiFi.begin("esp_pulseox");
      WiFi.softAP("pulseox8266");
      break;
    }
  }


  Serial.println();
  Serial.println(F("WiFi connected"));

  //ArduinoOTA.begin();

  LittleFS.begin();
  //ws.onEvent(ws_event);
  server.addHandler(&ws);
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.htm");
  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/www/404.htm");
  });
  server.begin();

  Serial.print(F("Server running on: http://"));
  Serial.println(WiFi.localIP());
}

void loop() {
  particleSensor.check();  //Check the sensor, read up to 3 samples

  while (particleSensor.available())  //do we have new data?
  {
    auto red = particleSensor.getFIFORed();
    auto ir = particleSensor.getFIFOIR();

    char data[32];
    snprintf(data, sizeof(data) - 2, "{\"red\":%d,\"ir\":%d}", red, ir);

    Serial.println(data);
    ws.textAll(data);

    static const byte RATE_SIZE = 4;  //Increase this for more averaging. 4 is good.
    static byte rates[RATE_SIZE];     //Array of heart rates
    static byte rateSpot = 0;
    static uint64_t lastBeat = 0;  //Time at which the last beat occurred

    if (checkForBeat(ir) == true) {
      //We sensed a beat!
      uint64_t delta = millis() - lastBeat;
      lastBeat = millis();

      float beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;  //Store this reading in the array
        rateSpot %= RATE_SIZE;                     //Wrap variable

        //Take average of readings
        uint beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;

        String bd = "{\"bpm\":";
        bd += beatAvg;
        bd += "}";
        Serial.println(bd);
        ws.textAll(bd);
      }
    }

    particleSensor.nextSample();
  }

  //ArduinoOTA.handle();
  //ws.cleanupClients();
}
