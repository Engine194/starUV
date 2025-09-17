#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"


int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 5;

const char *ssid = "Engine194";
const char *password = "Engine194";

WebServer server(80);

const char *www_username = "admin";
const char *www_password = "esp32";

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void setup() {
  Serial.begin(115200);

  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1);
  }
  IPAddress myIP = WiFi.softAPIP();
  ArduinoOTA.begin();


  server.on("/", []() {
    readFile(SD, "/admin.env.json");
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    server.send(200, "text/plain", "Login OK");
  });
  server.begin();

  Serial.print("Open http://");
  Serial.print(myIP);
  Serial.println("/ in your browser to see it working");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  delay(2);  //allow the cpu to switch to other tasks
}
