#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include "Adafruit_MCP23017.h"

#define mPS 0.0002500026
#define bPS 0.0347121629

#define mBATT 0.0002520267
#define bBATT 0.0196946882


#include "wificred.h"
/* what's in wificred.h 
const char* ssid = ".....";
const char* password = ".....";
*/

uint32_t timeout;
uint32_t sleepTimeout;

#define LED 0

#define HOSTNAME "solohm-mm-"
#define BROADCASTINTERVAL 2000

#define SLEEPINTERVAL 60
#define RUNNINGINTERVAL 15

WiFiUDP udp;
unsigned int port = 12345;
IPAddress broadcastIp;

ESP8266WebServer server(80);

Adafruit_MCP23017 mcp;

#define CS_ADC0 2
// static SPISettings spiSettings;
void adcSetup();
void adcShutdown();
void adcSetupRead();
void adcRead();
uint16_t vmax,vmin,imax,imin;
int reads;
int32_t adc0Average,adc1Average,adc2Average,adc3Average;
int32_t sum0, sum1, sum2, sum3;
int16_t d4[4];


uint8_t settingDebug = 0;
uint16_t settingSamples = 0;

void shutdown();
void adcAverage(int samples);

void broadcast(char *message) {
  udp.beginPacketMulticast(broadcastIp, port, WiFi.localIP());
  udp.write(message);
  udp.endPacket();
}

void handleRoot() {
  String html("<head><meta http-equiv='refresh' content='5'></head><body>You are connected</><br><b>");
  html.concat(millis());
  server.send(200, "text/html", html.c_str());
}

void handleSetting() {
    char buf[100];
    if(server.hasArg("debug")) {
      settingDebug = server.arg("debug").toInt();
    }
    if(server.hasArg("samples")) {
      settingSamples = server.arg("samples").toInt();
      if (settingSamples > 5000) {
        settingSamples = 5000;
      }
    }
    sprintf(buf,"{\"debug\":%d,\"samples\":%d}",settingDebug,settingSamples);
    server.send(200, "text/plain", buf);
}

void handleData() {
  char buf[100];

  if(server.hasArg("samples")) {
    settingSamples = server.arg("samples").toInt();
  }

  adcAverage(settingSamples);
  sprintf(buf,"%d,%d,%d,%d\n",adc0Average,adc1Average,adc2Average,adc3Average);
  server.send(200, "text/plain", buf);
}

void broadcastStatus(char *state) {
  uint32_t start = millis();

  adcAverage(settingSamples);

  if (settingDebug > 0) Serial.printf("%d %ld\n",settingSamples,millis()-start);

  String message("{\"id\":");

  message.concat("\"");
  message.concat(HOSTNAME);
  message.concat(String(ESP.getChipId(),HEX));
  message.concat("\"");
  message.concat(",\"uptime\":");
  message.concat(millis());
  message.concat(",\"class\":\"");
  message.concat(MAIN_NAME);
  message.concat("\",\"ipaddress\":\"");
  message.concat(WiFi.localIP().toString());

  message.concat("\",\"average.adc0\":");
  message.concat(adc0Average);
  message.concat(",\"average.adc1\":");
  message.concat(adc1Average);
  message.concat(",\"average.adc2\":");
  message.concat(adc2Average);
  message.concat(",\"average.adc3\":");
  message.concat(adc3Average);


  message.concat(",\"state\":\"");
  message.concat(state);
  message.concat("\"");

  message.concat("}");

  broadcast((char *)message.c_str());
  
  Serial.println(message);
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
        case WIFI_EVENT_STAMODE_GOT_IP:
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case WIFI_EVENT_STAMODE_DISCONNECTED:
            Serial.println("WiFi lost connection");
            break;
    }
}

void setup() {
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  Serial.print("\n\n");
  Serial.print(MAIN_NAME);
  Serial.println(" setup");

  WiFi.onEvent(WiFiEvent);
  Serial.println("setting mode WIFI_STA");
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.print("connecting to ");
  Serial.print(ssid);
  Serial.print(" ");
  Serial.println(password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  // WiFi.softAP((const char *)hostname.c_str());

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("arduinoOTA start");
    digitalWrite(LED, HIGH);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\arduinoOTA end");
    digitalWrite(LED, HIGH);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    digitalWrite(LED, !digitalRead(LED));

  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  Serial.print("IP address ");
  Serial.print(WiFi.localIP());
  Serial.print(" ");
  Serial.print(hostname);
  Serial.println(".local");

  broadcastIp = ~WiFi.subnetMask() | WiFi.gatewayIP();
  Serial.print("broadcast address: ");
  Serial.println(broadcastIp);

  udp.begin(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/setting", handleSetting);

  server.begin();
  Serial.println("HTTP server started");

  mcp.begin();      // use default address 0
  mcp.pinMode(11, OUTPUT);
  mcp.pinMode(14, OUTPUT);
  mcp.pinMode(15, OUTPUT);

  mcp.digitalWrite(11, LOW); // green led on


  mcp.digitalWrite(14, HIGH);
  mcp.digitalWrite(15, HIGH);

  pinMode(CS_ADC0, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  digitalWrite(CS_ADC0, HIGH);


  adcSetup();
  adcSetupRead();

  mcp.digitalWrite(14, LOW); // set charge current 
  mcp.digitalWrite(15, HIGH);


  mcp.pinMode(5, OUTPUT);
  mcp.digitalWrite(5, LOW); // highest current gain
  mcp.pinMode(6, OUTPUT);
  mcp.digitalWrite(6, HIGH);

  mcp.pinMode(0, OUTPUT);
  mcp.pinMode(1, OUTPUT);
  mcp.pinMode(2, OUTPUT);
  mcp.pinMode(3, OUTPUT);
  mcp.pinMode(4, OUTPUT);

  mcp.digitalWrite(0, HIGH); 
  mcp.digitalWrite(1, LOW); 
  mcp.digitalWrite(2, LOW); 
  mcp.digitalWrite(3, LOW); 
  mcp.digitalWrite(4, LOW); 

  sleepTimeout = millis() + RUNNINGINTERVAL*1000L;

  settingDebug=0;
  settingSamples = 100;

  adcAverage(1000);
  broadcastStatus((char *)"setup");

}

void loop() {
  ArduinoOTA.handle();
  yield();
  server.handleClient();
  if (millis() > timeout) {
    digitalWrite(LED, !digitalRead(LED));
    broadcastStatus((char *)"running");
    timeout = millis() + BROADCASTINTERVAL;
  }
}

void shutdown() {
    adcShutdown();

    sleepTimeout = millis() + 10000L;
    broadcastStatus((char *)"shutdown");
    while (millis() < sleepTimeout) {
      ArduinoOTA.handle();
      yield();
      server.handleClient();
    }

    broadcastStatus((char *)"sleeping");
    Serial.println("Going to sleep");
    mcp.digitalWrite(11, HIGH); // green led off
    ESP.deepSleep(SLEEPINTERVAL * 1000000L,WAKE_RF_DEFAULT);
}


void adcSetup() {
  int i;
  uint8_t b;
  digitalWrite(SCK, HIGH);
  digitalWrite(MOSI, LOW);
  digitalWrite(CS_ADC0, LOW);

  b = B01100000; 
  for (i = 0; i < 8; i++) {
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }

  b = 0x11;
  for (i = 0; i < 8; i++) {
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }
  digitalWrite(CS_ADC0, HIGH);
}


void adcShutdown() {
  int i;
  uint8_t b;
  digitalWrite(SCK, HIGH);
  digitalWrite(MOSI, LOW);
  digitalWrite(CS_ADC0, LOW);

  b = B01100000; // write conf reg
  for (i = 0; i < 8; i++) {
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }

  b = 0x80; // set shutdown bit
  for (i = 0; i < 8; i++) {
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }
  digitalWrite(CS_ADC0, HIGH);
}

void adcSetupRead() {
  int i;
  uint8_t b;
  digitalWrite(SCK, HIGH);
  digitalWrite(MOSI, LOW);
  digitalWrite(CS_ADC0, LOW);

  b = B11100000; 
  for (i = 0; i < 8; i++) {
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }

  for (i = 0; i < 8; i++) {
    digitalWrite(SCK, LOW);
    // Serial.print(digitalRead(MISO));
    digitalWrite(SCK, HIGH);
  }
  // Serial.println();
  digitalWrite(CS_ADC0, HIGH);
}


void adcRead() {
  int i,c;
  uint32_t d;
  uint8_t b;

  digitalWrite(SCK, HIGH);
  digitalWrite(MOSI, LOW);
  digitalWrite(CS_ADC0, LOW);

  b = B11110000; 
  for (i = 0; i < 8; i++) {
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }

  c = 0; 
  for (i = 0; i < 96; i++) {
    digitalWrite(SCK, LOW);
    d |= digitalRead(MISO);
    digitalWrite(SCK, HIGH);
    d <<= 1;

    if ((i % 24) == 23) {
      // Serial.printf("%08X\n",d>>1);
      d4[c++] = (int16_t)((d >> 9)&0xFFFF); // shift values to correct location
      d = 0;
    }
  }
  digitalWrite(CS_ADC0, HIGH);

  if (settingDebug > 1) Serial.printf("d4 d %7d %7d %7d %7d",d4[0],d4[1],d4[2],d4[3]);
  if (settingDebug > 2) Serial.printf("d4 x %07X %07X %07X %07X",d4[0],d4[1],d4[2],d4[3]);
  if (settingDebug > 1) Serial.println();
}

void adcAverage(int samples) {
  // d4 values are 15 bit max, shouldn't overflow for small count
  sum0 = 0;
  sum1 = 0;
  sum2 = 0;
  sum3 = 0;

  for (int i = 0; i < samples; i++) {
    adcRead();
    sum0 = sum0 + d4[0];
    sum1 = sum1 + d4[1];
    sum2 = sum2 + d4[2];
    sum3 = sum3 + d4[3];
  }
  if (settingDebug > 0) Serial.printf("sum  %7d %7d %7d %7d\n",sum0,sum1,sum2,sum3);
  adc0Average = round(float(sum0) / samples);
  adc1Average = round(float(sum1) / samples);
  adc2Average = round(float(sum2) / samples);
  adc3Average = round(float(sum3) / samples);
  if (settingDebug > 0) Serial.printf("ave  %7d %7d %7d %7d\n\n",adc0Average,adc1Average,adc2Average,adc3Average);
}
