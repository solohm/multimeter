#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
// #include <SPI.h>

const char* ssid = "ssid";
const char* password = "p*ssword";
uint32_t timeout;
uint32_t seq;

#define LED 0
#define CS_ADC0 2
// static SPISettings spiSettings;

#define HOSTNAME "solohm-mm-"
#define BROADCASTINTERVAL 1000

WiFiUDP udp;
unsigned int port = 12345;
IPAddress broadcastIp;

ESP8266WebServer server(80);

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
  message.concat(",\"id\":");
  message.concat(seq++);
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
  pinMode(CS_ADC0, OUTPUT);

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

  //SPI.begin();
  // digitalWrite(CS_ADC0, HIGH);
  //spiSettings = SPISettings(100000, MSBFIRST, SPI_MODE2);


  pinMode(CS_ADC0, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  digitalWrite(CS_ADC0, HIGH);

}

#define REPLYLEN 4

/*
void adcQuery() {
  uint8_t buf[20];
  int i;
  uint8_t b = 0xF0; // data reg
  SPI.beginTransaction(spiSettings);
  digitalWrite(CS_ADC0, LOW);
  SPI.transfer(B0110000);
  SPI.transfer(B0001000);
  SPI.transferBytes(NULL,buf,REPLYLEN);
  digitalWrite(CS_ADC0, HIGH);
  SPI.endTransaction();
  for (i = 0; i < REPLYLEN; i++) {
    Serial.println(buf[i],HEX);
  }
  delay(2000);
}
*/

void adcSetup() {
  int i;
  uint8_t b;
  digitalWrite(SCK, HIGH);
  digitalWrite(MOSI, LOW);
  delay(1);
  digitalWrite(CS_ADC0, LOW);

  b = B01100000; 
  for (i = 0; i < 8; i++) {
    delay(1);
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    delay(1);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }

  b = 0x11;
  for (i = 0; i < 8; i++) {
    delay(1);
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    delay(1);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }
  delay(1);
  digitalWrite(CS_ADC0, HIGH);
}

void adcSetupRead() {
  int i;
  uint8_t b;
  digitalWrite(SCK, HIGH);
  digitalWrite(MOSI, LOW);
  delay(1);
  digitalWrite(CS_ADC0, LOW);

  b = B11100000; 
  for (i = 0; i < 8; i++) {
    delay(1);
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
    delay(1);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }

  for (i = 0; i < 8; i++) {
    delay(1);
    digitalWrite(SCK, LOW);
    delay(1);
    Serial.print(digitalRead(MISO));
    digitalWrite(SCK, HIGH);
  }
  Serial.println();
  delay(1);
  digitalWrite(CS_ADC0, HIGH);
}



  

void loop() {
  ArduinoOTA.handle();
  yield();
  server.handleClient();
  if (millis() > timeout) {
    digitalWrite(LED, !digitalRead(LED));
    timeout = millis() + BROADCASTINTERVAL;
    broadcastStatus();
    //adcQuery();
    adcSetup();
    adcSetupRead();
  }
}
