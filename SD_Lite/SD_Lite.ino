#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <SHA1Builder.h>

int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 5;

String secret_token_hex = "5715790a892990382d98858c4aa38d0617151575";

const char *ssid = "Engine194";
const char *password = "Engine194";

static unsigned char _bearer[20];

WebServer server(80);

const char *www_username = "admin";
const char *www_password = "6adfb183a4a2c94a2f92dab5ade762a47889a5a1";

String *check_bearer_or_auth(HTTPAuthMethod mode, String authReq, String params[]) {
  // we expect authReq to be "bearer some-secret"
  String lcAuthReq = authReq;
  lcAuthReq.toLowerCase();
  if (mode == OTHER_AUTH && (lcAuthReq.startsWith("bearer "))) {
    String secret = authReq.substring(7);
    secret.trim();

    uint8_t sha1[20];
    SHA1Builder sha_builder;

    sha_builder.begin();
    sha_builder.add((uint8_t *)secret.c_str(), secret.length());
    sha_builder.calculate();
    sha_builder.getBytes(sha1);

    if (memcmp(_bearer, sha1, sizeof(_bearer)) == 0) {
      Serial.println("Bearer token matches");
      return new String("anything non null");
    } else {
      Serial.println("Bearer token does not match");
    }
  } else if (mode == BASIC_AUTH) {
    bool ret = server.authenticateBasicSHA1(www_username, www_password);
    if (ret) {
      Serial.println("Basic auth succeeded");
      return new String(params[0]);
    } else {
      Serial.println("Basic auth failed");
    }
  }

  // No auth found
  return NULL;
};

void readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}

void setup()
{
  Serial.begin(115200);

  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin())
  {
    Serial.println("Card Mount Failed");
    return;
  }
  if (!WiFi.softAP(ssid, password))
  {
    log_e("Soft AP creation failed.");
    while (1);
  }
  
  IPAddress myIP = WiFi.softAPIP();
  ArduinoOTA.begin();

  // Convert token to a convenient binary representation.
  size_t len = HEXBuilder::hex2bytes(_bearer, sizeof(_bearer), secret_token_hex);
  if (len != 20) {
    Serial.println("Bearer token does not look like a valid SHA1 hex string ?!");
  }

  server.on("/", HTTP_GET, []()
    {
      if (!server.authenticate(&check_bearer_or_auth)) {
        Serial.println("No/failed authentication");
        return server.requestAuthentication();
      }
      Serial.println("Authentication succeeded");
      File file = SD.open("/ui/index.html", "r");
      if (file) {
          server.streamFile(file, "text/html");
          file.close();
      } else {
          server.send(404, "text/plain", "Page Not Found");
      }
    });

  server.serveStatic("/", SD, "/ui/");
  server.begin();

  Serial.print("Open http://");
  Serial.print(myIP);
  Serial.println("/ in your browser to see it working");
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
  delay(2); // allow the cpu to switch to other tasks
}
