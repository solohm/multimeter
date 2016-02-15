#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "ssid";
const char* password = "password";
uint32_t timeout;

#define HOSTNAME "solohm-mm-"

WiFiUDP udp;
unsigned int port = 12345;
IPAddress broadcastIp;

void broadcast(char * message) {
  udp.beginPacketMulticast(broadcastIp, port, WiFi.localIP());
  udp.write(message);
  udp.endPacket();
}

void broadcastStatus() {
  String message("{\"nodeid\":");

  message.concat("\"");
  message.concat(HOSTNAME);
  message.concat(String(ESP.getChipId(),HEX));
  message.concat("\"");
  message.concat(",\"uptime\":");
  message.concat(millis());
  message.concat("}");

  broadcast((char *)message.c_str());
  
  Serial.println(message);
}
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println("BasicOTA setup");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  ArduinoOTA.setHostname((const char *)hostname.c_str());


  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("arduinoOTA start");
    digitalWrite(LED_BUILTIN, HIGH);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\arduinoOTA end");
    digitalWrite(LED_BUILTIN, HIGH);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

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
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(hostname);

  broadcastIp = ~WiFi.subnetMask() | WiFi.gatewayIP();
  Serial.print("broadcast address: ");
  Serial.println(broadcastIp);



  udp.begin(WiFi.localIP());
  
  broadcast((char *)"setup");
}

void loop() {
  ArduinoOTA.handle();
  yield();
  if (millis() > timeout) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    timeout = millis() + 1000;
    broadcastStatus();
  }

}
