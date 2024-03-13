#include <Wire.h>
#include <MAX30105.h>
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
  byte sampleAverage = 16;    //Options: 1, 2, 4, 8, 16, 32
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


const float mid_smooth = 0.95;
float red_mid = 50000;
float ir_mid = 50000;
uint64_t last_low_time = 0;
int32_t ir_min = 0, ir_max = 0;
float minmax_smooth = 0.95;
float hr_detect_fac = 0.75;
void loop() {
  particleSensor.check();  //Check the sensor, read up to 3 samples

  while (particleSensor.available())  //do we have new data?
  {
    const uint32_t red = particleSensor.getFIFORed();
    const uint32_t ir = particleSensor.getFIFOIR();

    //normalized values
    if (abs(ir - ir_mid) > 1000) ir_mid = ir;
    if (abs(red - red_mid) > 1000) red_mid = red;

    red_mid = red_mid * mid_smooth + red * (1.0 - mid_smooth);
    ir_mid = ir_mid * mid_smooth + ir * (1.0 - mid_smooth);

    int32_t norm_red = red - red_mid;
    int32_t norm_ir = ir - ir_mid;

    char data[128];
    snprintf(data, sizeof(data) - 2, "{\"red\":%d,\"mid_red\":%d,\"ir\":%d,\"mid_ir\":%d}", norm_red, uint32_t(red_mid), norm_ir, uint32_t(ir_mid));

    Serial.println(data);
    ws.textAll(data);

    ir_max = ir_max * minmax_smooth;
    ir_max = max(ir_max, norm_ir);
    ir_min = ir_min * minmax_smooth;
    ir_min = min(ir_min, norm_ir);

    if (last_low_time == 0)
      if (norm_ir < ir_min * hr_detect_fac) {
        last_low_time = millis();
      }

      else if (last_low_time != 0)
        if (norm_ir > ir_max * hr_detect_fac) {
          uint64_t delta = millis() - last_low_time;
          last_low_time = 0;            //prevent multi-trig
          float bpm = 60000.0 / delta;  //1.0 / (delta /*ms*/ * 0.001 /*s/ms*/) * 60.0 /*s/min*/;
          if (30 < bpm && bpm < 250) {
            String bd = "{\"bpm\":";
            bd += bpm;
            bd += "}";
            Serial.println(bd);
            ws.textAll(bd);
          }
        }

    particleSensor.nextSample();
  }

  static uint64_t last_slow = 0;
  if (millis() - last_slow > 1000) {
    last_slow = millis();
    char buf[64];
    snprintf(buf, sizeof(buf) - 2, "{\"temp\":%.1f,\"rssi\":%d}", particleSensor.readTemperature(), WiFi.RSSI());
    Serial.println(buf);
    ws.textAll(buf);
    ws.cleanupClients();
  }


  //ArduinoOTA.handle();
}
