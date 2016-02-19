#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

#include <Wire.h>
#include <DS1337.h>

#include <DS1337.h>

DS1337 rtc;

const char* ssid = "ssid";
const char* password = "p*ssword";
uint32_t timeout;

#define LED 0

#define HOSTNAME "solohm-mm-"
#define BROADCASTINTERVAL 1000

WiFiUDP udp;
WiFiUDP ntpUdp;

unsigned int port = 12345;
IPAddress broadcastIp;

ESP8266WebServer server(80);

unsigned long sendNTPpacket(IPAddress& address);
void timeGet();

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


  rtc.init();
  rtc.clearAlarm();
  rtc.resetTick();
  ntpUdp.begin(WiFi.localIP());



}

void loop() {
  ArduinoOTA.handle();
  yield();
  server.handleClient();
  if (millis() > timeout) {
    digitalWrite(LED, !digitalRead(LED));
    timeout = millis() + BROADCASTINTERVAL;
    broadcastStatus();

    Serial.print("CLOCK: ");
    Serial.print(rtc.getDate().getTimeString());
    Serial.print(" - ");
    Serial.print(rtc.getDate().getDateString());
/*
    if (!rtc.isRunning()) {
      Serial.print(" STOPPED");
    } else {
      Serial.print(" RUNNING");
    }
    if (rtc.hasStopped()) {
      Serial.print(" HAS STOPPED");
    }
*/
    Serial.println();


    timeGet();
  }

}


// from esp8266/Arduino/libraries/ESP8266WiFi/examples/NTPClient
unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServerIP; // time.nist.gov NTP server address
IPAddress timeServer(128,138,141,172);
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

uint32_t timeoutTimeGet;

// yy-mm-dd hh:mm

void timeGet() {
  if (millis() > timeoutTimeGet) {
    Serial.print("timeGet from ");
    Serial.println(ntpServerName);

    //WiFi.hostByName(ntpServerName, timeServerIP); 
    //sendNTPpacket(timeServerIP); 
    sendNTPpacket(timeServer); 

    Serial.print("Requesting from ");
    Serial.println(timeServer);
    timeoutTimeGet = millis() + 5000;
  }

  int cb = ntpUdp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    ntpUdp.read(packetBuffer, NTP_PACKET_SIZE); 

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
}

unsigned long sendNTPpacket(IPAddress& address) {
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  ntpUdp.beginPacket(address, 123); //NTP requests are to port 123
  ntpUdp.write(packetBuffer, NTP_PACKET_SIZE);
  ntpUdp.endPacket();
}
