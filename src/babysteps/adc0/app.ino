#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
// #include <SPI.h>
#include "Adafruit_MCP23017.h"

const char* ssid = "ssid";
const char* password = "p*ssword";
uint32_t timeout;
uint32_t seq;

uint16_t d4[4];

Adafruit_MCP23017 mcp;

#define VS60    0
#define VS120   1
#define VS350   2
#define VS625   3
#define VS1550  4
#define IS2     5
#define IS10    6
#define IS20    7

#define LED 0
#define CS_ADC0 2
// static SPISettings spiSettings;
void adcSetup();
void adcSetupRead();

#define HOSTNAME "solohm-mm-"
#define BROADCASTINTERVAL 1000

WiFiUDP udp;
unsigned int port = 12345;
IPAddress broadcastIp;

uint16_t vmax,vmin,imax,imin;

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

void dataReady() {
  Serial.println("dataReady");
}
  

void broadcastStatus() {
  return;
  String message;
  char buf[50];

  sprintf(buf,"{\"adc0v\":%d",d4[0]);
  message.concat(buf);
  sprintf(buf,",\"adc0i\":%d",d4[1]);
  message.concat(buf);
  sprintf(buf,",\"adc0batt\":%d",d4[2]);
  message.concat(buf);
  sprintf(buf,",\"adc0ps\":%d",d4[3]);
  message.concat(buf);

  message.concat(",\"nodeid\":");
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
  // pinMode(LED, OUTPUT);
  pinMode(LED, INPUT);
  pinMode(CS_ADC0, OUTPUT);

  Serial.begin(115200);
  Serial.print("\n\n");
  Serial.print(MAIN_NAME);
  Serial.println(" setup");

//  WiFi.mode(WIFI_OFF);

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
    //digitalWrite(LED, HIGH);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\arduinoOTA end");
    // digitalWrite(LED, HIGH);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    // digitalWrite(LED, !digitalRead(LED));

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

  adcSetup();
  adcSetupRead();

  mcp.begin();      // use default address 0


  mcp.pinMode(VS60,   OUTPUT);
  mcp.pinMode(VS120,  OUTPUT);
  mcp.pinMode(VS350,  OUTPUT);
  mcp.pinMode(VS625,  OUTPUT);
  mcp.pinMode(VS1550, OUTPUT);
  mcp.pinMode(IS2,    OUTPUT);
  mcp.pinMode(IS10,   OUTPUT);
  mcp.pinMode(IS20,   OUTPUT);

  mcp.digitalWrite(VS60,    HIGH);
  mcp.digitalWrite(VS120,   LOW);
  mcp.digitalWrite(VS350,   LOW);
  mcp.digitalWrite(VS625,   LOW);
  mcp.digitalWrite(VS1550,  LOW);
  mcp.digitalWrite(IS2,     LOW);
  mcp.digitalWrite(IS10,    HIGH);
  mcp.digitalWrite(IS20,    LOW);

  vmax = 0;
  vmin = 30000;
  imax = 0;
  imin = 30000;

  attachInterrupt(0,dataReady,FALLING);
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
 // delay(1);
  digitalWrite(CS_ADC0, LOW);

  b = B01100000; 
  for (i = 0; i < 8; i++) {
  //  delay(1);
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
  //  delay(1);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }

  b = 0x11;
  for (i = 0; i < 8; i++) {
  //  delay(1);
    digitalWrite(MOSI, (b & 0x80));
    digitalWrite(SCK, LOW);
  //  delay(1);
    digitalWrite(SCK, HIGH);
    b <<= 1;
  }
  //delay(1);
  digitalWrite(CS_ADC0, HIGH);
}

void adcSetupRead() {
  int i;
  uint8_t b;
  digitalWrite(SCK, HIGH);
  digitalWrite(MOSI, LOW);
  //delay(1);
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
    Serial.print(digitalRead(MISO));
    digitalWrite(SCK, HIGH);
  }
  Serial.println();
  digitalWrite(CS_ADC0, HIGH);
}

int reads;

#define BURSTSIZE 250
void adcReadBurst() {
  uint32_t voltages[BURSTSIZE],currents[BURSTSIZE];
  int i,c;
  uint32_t d;
  uint8_t b;
  char buf[100];


  for (c = 0; c < BURSTSIZE; c++) {
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

    d = 0;
    for (i = 0; i < 24; i++) {
      digitalWrite(SCK, LOW);
      d |= digitalRead(MISO);
      digitalWrite(SCK, HIGH);
      d <<= 1;
    }
    voltages[c] = d;

    d = 0;
    for (i = 0; i < 24; i++) {
      digitalWrite(SCK, LOW);
      d |= digitalRead(MISO);
      digitalWrite(SCK, HIGH);
      d <<= 1;
    }
    currents[c] = d;
    digitalWrite(CS_ADC0, HIGH);
  }

  vmax = 0;
  vmin = 32000;
  imax = 0;
  imin = 32000;

  // Serial.println("\n\nvoltage,vmin,vmax,vdiff,current,cmin,cmax,cdiff");
  for (c = 0; c < BURSTSIZE; c++) {
    voltages[c] = (voltages[c] >> 9)&0xFFFF;
    currents[c] = (currents[c] >> 9)&0xFFFF;

    if (voltages[c] > vmax) vmax = voltages[c];
    if (voltages[c] < vmin) vmin = voltages[c];
    if (currents[c] > imax) imax = currents[c];
    if (currents[c] < imin) imin = currents[c];

    sprintf(buf,"%7d %7d %7d %7d, %7d %7d %7d %7d",voltages[c],vmin,vmax,vmax-vmin,currents[c],imin,imax,imax-imin); 
    Serial.println(buf);
  }
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
    // Serial.print(digitalRead(MISO));
    d |= digitalRead(MISO);
    digitalWrite(SCK, HIGH);
    d <<= 1;

/*
    if ((i % 24) == 15) {
      Serial.print(" ");
    }
*/

    if ((i % 24) == 23) {
      d4[c++] = (d >> 9)&0xFFFF; // shift values to correct location
      d = 0;
//      Serial.println();
    }
  }
  digitalWrite(CS_ADC0, HIGH);

  if (d4[0] > vmax) vmax = d4[0];
  if (d4[0] < vmin) vmin = d4[0];
  if (d4[1] > imax) imax = d4[1];
  if (d4[1] < imin) imin = d4[1];

  char buf[100];
  sprintf(buf,"%7d %7d %7d %7d, %7d %7d %7d %7d",d4[0],vmin,vmax,vmax-vmin,d4[1],imin,imax,imax-imin); 
  Serial.println(buf);


  if ((reads % 100) == 98) {
    Serial.println();
    delay(2000);
  }

  if ((reads++ % 100) == 0) {
    vmax = d4[0];
    vmin = d4[0];
    imax = d4[1];
    imin = d4[1];
  }

/*
  for (i = 0; i < 4; i++) {
    char buf[100];
    sprintf(buf,"%7d",d4[i],d4[i]); 
    Serial.print(buf);
    if (i == 0) {
      sprintf(buf,"%7d %7d %d",vmin,vmax,vmax-vmin); 
      Serial.print(buf);
    }
    if (i == 1) {
      sprintf(buf,"%7d %7d %d",imin,imax,imax-imin); 
      Serial.print(buf);
    }
    Serial.println();
  }
*/

}


  

void loop() {
  ArduinoOTA.handle();
  yield();
  server.handleClient();
  if (millis() > timeout) {
    adcReadBurst();
    // digitalWrite(LED, !digitalRead(LED));
    timeout = millis() + BROADCASTINTERVAL;
    broadcastStatus();
  }
}
