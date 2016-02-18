#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include "LM75.h"


const char* ssid = "ssid";
const char* password = "p*ssword";
uint32_t timeout;

#define LED 0

#define HOSTNAME "solohm-mm-"
#define BROADCASTINTERVAL 1000

WiFiUDP udp;
unsigned int port = 12345;
IPAddress broadcastIp;

ESP8266WebServer server(80);

LM75 tempSensor;

void broadcast(char * message) {
  udp.beginPacketMulticast(broadcastIp, port, WiFi.localIP());
  udp.write(message);
  udp.endPacket();
}

void handleRoot() {
  String html("<head><meta http-equiv='refresh' content='5'></head><body>You are connected</><br><b>");
  html.concat(millis());
  html.concat("</b><form action='reset'><input type='submit' value='Reset'></form></body>");
  server.send(200, "text/html", html.c_str());
}

void handleReset() {
  server.send(200, "text/html", "<b>Reseting</b>");
  ESP.reset();
}

void broadcastStatus() {
  String message("{\"nodeid\":");

  message.concat("\"");
  message.concat(HOSTNAME);
  message.concat(String(ESP.getChipId(),HEX));
  message.concat("\"");
  message.concat(",\"temperature\":");
  message.concat(tempSensor.temp());
  message.concat(",\"uptime\":");
  message.concat(millis());
  message.concat(",\"class\":\"");
  message.concat(MAIN_NAME);
  message.concat("\",\"ipaddress\":\"");
  message.concat(WiFi.localIP().toString());
  message.concat("\"}");

  broadcast((char *)message.c_str());
  
  Serial.println(message);
}

void setup() {
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  Serial.print("\n\n");
  Serial.print(MAIN_NAME);
  Serial.println(" setup");

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
  WiFi.softAP((const char *)hostname.c_str());

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
  server.on("/reset", handleReset);

  server.begin();
  Serial.println("HTTP server started");

  Wire.begin(4,5);

}

void loop() {
  ArduinoOTA.handle();
  yield();
  server.handleClient();
  if (millis() > timeout) {
    digitalWrite(LED, !digitalRead(LED));
    timeout = millis() + BROADCASTINTERVAL;
    broadcastStatus();
  }

}
