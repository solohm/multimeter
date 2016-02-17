#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(81);

#define USE_SERIAL Serial

ESP8266WebServer server(80);

void handleCss() {
  String html("html { font-family: Helvetica, Arial, sans-serif; font-size: 100%; background: #333; }");
  server.send(200, "text/html", html.c_str());
}

void handleRoot() {
  String html("<!DOCTYPE html>\
<html lang='en'>\
<head>\
    <meta charset='utf-8'>\
    <title>WebSockets Demo</title>\
    <link rel='stylesheet' href='style.css'>\
</head>\
<body>\
    <div id='page-wrapper'>\
        <h1>WebSockets Demo</h1>\
        <div id='status'>Connecting...</div>\
        <ul id='messages'></ul>\
        <form id='message-form' action='#' method='post'>\
            <textarea id='message' placeholder='Write your message here...' required></textarea><br/>\
            <button type='submit'>Send Message</button>\
            <button type='button' id='close'>Close Connection</button>\
        </form>\
    </div>\
\
    <script src='app.js'></script>\
</body>\
</html>");
  server.send(200, "text/html", html.c_str());
}

void handleJS() {
String html("window.onload = function() {\
  var form = document.getElementById('message-form');\
  var messageField = document.getElementById('message');\
  var messagesList = document.getElementById('messages');\
  var socketStatus = document.getElementById('status');\
  var closeBtn = document.getElementById('close');\
  var socket = new WebSocket('ws://IPADDRESS:81');\
  socket.onerror = function(error) {\
    console.log('WebSocket Error: ' + error);\
  };\
  socket.onopen = function(event) {\
    console.log(event);\
    socketStatus.innerHTML = 'Connected to: ' + event.currentTarget.url;\
    socketStatus.className = 'open';\
  };\
  socket.onmessage = function(event) {\
    var message = event.data;\
    messagesList.innerHTML = '<li class=\"received\"><span>Received:</span>' + message + '</li>';\
  };\
  socket.onclose = function(event) {\
    socketStatus.innerHTML = 'Disconnected from WebSocket.';\
    socketStatus.className = 'closed';\
  };\
  form.onsubmit = function(e) {\
    e.preventDefault();\
    var message = messageField.value;\
    socket.send(message);\
    messagesList.innerHTML += '<li class=\"sent\"><span>Sent:</span>' + message + '</li>';\
    messageField.value = '';\
    return false;\
  };\
  closeBtn.onclick = function(e) {\
    e.preventDefault();\
    socket.close();\
    return false;\
  };\
};");
  html.replace("IPADDRESS", WiFi.localIP().toString());
  server.send(200, "text/html", html.c_str());
}




void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
				
				// send message to client
				webSocket.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

            for (int i = 0; i < strlen((const char *)payload); i++) {
              payload[i] = toupper(payload[i]);
            }

            // send message to client
            //webSocket.sendTXT(num, payload);

            // send data to all connected clients
             webSocket.broadcastTXT(payload);
            break;
        case WStype_BIN:
            USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }

}

uint32_t timeout;

void setup() {
    // USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);

    //Serial.setDebugOutput(true);
    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFiMulti.addAP("zeeboo", "zaralily");

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    timeout = 0;

    server.on("/", handleRoot);
    server.on("/app.js", handleJS);
    server.on("/style.css", handleCss);
    server.begin();
}

void loop() {
    server.handleClient();
    webSocket.loop();
    if (millis() > timeout) {
      char buf[50];
      sprintf(buf,"%d",millis());
      webSocket.broadcastTXT(buf);
      timeout = millis() + 5000;
    }
    yield();
}
