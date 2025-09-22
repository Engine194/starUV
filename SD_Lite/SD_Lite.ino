#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <SHA1Builder.h>
#include <base64.hpp>
#include <ArduinoJson.h>


int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 5;

String secret_token_hex = "5715790a892990382d98858c4aa38d0617151575";

const char *ssid = "Engine194";
const char *password = "Engine194";

static unsigned char _bearer[20];

WebServer server(80);

struct PasswordStruct {
  const char* value;
  const char* date;
};

char www_username[127] = "admin";
char www_password[127];

int splitString(String inputString, char delimiter, String outputArray[], int arraySize) {
  int startIndex = 0;
  int endIndex = 0;
  int arrayIndex = 0;

  while (endIndex != -1 && arrayIndex < arraySize) {
    endIndex = inputString.indexOf(delimiter, startIndex);

    if (endIndex == -1) { // No more delimiters, get the rest of the string
      outputArray[arrayIndex++] = inputString.substring(startIndex);
    } else { // Delimiter found
      outputArray[arrayIndex++] = inputString.substring(startIndex, endIndex);
      startIndex = endIndex + 1;
    }
  }
  return arrayIndex; // Return the number of substrings found
}

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
    const char *username = (const char*)www_username;
    const char *password = (const char*)www_password;
    bool ret = server.authenticateBasicSHA1(username, password);
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

String extractUuidFromRequest(String authorization) {
  String rawCredential = authorization.substring(6);
  unsigned char base64[rawCredential.length() + 1];
  strcpy(reinterpret_cast<char*>(base64), rawCredential.c_str());
  unsigned char credential_c_str[255];
  unsigned int string_length = decode_base64(base64, credential_c_str);
  credential_c_str[string_length] = '\0';
  String credential = reinterpret_cast<char*>(credential_c_str);
  String splitedParams[2];
  splitString(credential, ':', splitedParams, 2);
  return splitedParams[0];
}

void readFile(fs::FS &fs, const char *path, char* result)
{
  Serial.printf("Reading file: %s\n", path);
  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  size_t bytesRead = 0;
  while (file.available()) {
    result[bytesRead] = (char)file.read();
    bytesRead++;
  }
  result[bytesRead] = '\0'; // Null-terminate the string
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

  server.on("/", HTTP_GET, []() // HTTP_POST HTTP_PUT HTTP_DELETE #CRUD - Create Read Update Delete // handler
    {
      strcpy(www_password, "");
      String uuid = extractUuidFromRequest(server.header("Authorization"));
      if (uuid.isEmpty()) {
        Serial.println("No/failed UUID");
        return server.requestAuthentication();
      } else {
        strcpy(www_username, uuid.c_str());
        char paths[] = "/auth/";
        strcat(paths, www_username);
        strcat(paths, ".json");
        char fileContent[1024];
        readFile(SD, paths, fileContent);
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, fileContent);

        // Check for parsing errors
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return server.requestAuthentication();
        }

        PasswordStruct password;
        password.value =  doc["passwords"][0]["value"];
        password.date = doc["passwords"][0]["date"];
        if (password.value != NULL && strlen(password.value) != 0) {
          strcpy(www_password, password.value);
        } else {
          Serial.println("No/failed UUID");
          return server.requestAuthentication();
        }

      }

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
