#include <Arduino.h>
 
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
 
// Replace with your network credentials
const char* ssid = "ESP32-Nikita";
const char* password = "esp32345678";
 
bool led1State = 0;
bool led2State = 0;
const int led1Pin = 18;
const int led2Pin = 19;
 
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
 
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Websocket Web Server Websocket</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>ESP WebSocket Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Khue Nguyen Creator</h2>
      <p class="state1">LED1: <span id="state1">%STATE1%</span></p>
      <p><button id="button1" class="button1">BUTTON1</button></p>
       <p class="state1">LED2: <span id="state2">%STATE2%</span></p>
      <p><button id="button2" class="button2">BUTTON2</button></p>
    </div>
  </div>
</body>
</html>
)rawliteral";
 
void notifyClients() {
  ws.textAll(String(led1State)+String(led2State));
}
 
 
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle1") == 0) {
      led1State = !led1State;
      notifyClients();
    }
    if (strcmp((char*)data, "toggle2") == 0) {
      led2State = !led2State;
      notifyClients();
    }
  }
}
 
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
 
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
 
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE1"){
    if (led1State){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  if(var == "STATE2"){
    if (led2State){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
}
 
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
 
  pinMode(led1Pin, OUTPUT);
  digitalWrite(led1Pin, LOW);
  pinMode(led2Pin, OUTPUT);
  digitalWrite(led2Pin, LOW);
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
 
  initWebSocket();
 
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
 
  // Start server
  server.begin();
}
 
void loop() {
  ws.cleanupClients();
  digitalWrite(led1Pin, led1State);
  digitalWrite(led2Pin, led2State);
}