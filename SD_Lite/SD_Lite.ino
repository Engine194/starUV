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
#include <ThreeWire.h>
#include <RtcDS1302.h>

int sck = 18;
int miso = 19;
int mosi = 23;
int cs = 5;

//connect Time module
const int IO = 27;    // DAT
const int SCLK = 14;  // CLK
const int CE = 26;    // RST

ThreeWire myWire(IO, SCLK, CE);
RtcDS1302<ThreeWire> Rtc(myWire);

String secret_token_hex = "5715790a892990382d98858c4aa38d0617151575";

const char *ssid = "Engine194";
const char *password = "Engine194";
const char *UNAUTHORIZED = "Unauthorized";
const char *INTERNAL_SERVER_ERROR = "Internal server error";
const char *BAD_REQUEST = "Bad request";

static unsigned char _bearer[20];

WebServer server(80);

struct PasswordStruct {
  const char* value;
  const char* date;
};

struct CycleStruct {
  char* start;
  char* end;
  int day[7];
  int fan_enable;
  int fan_delay;
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

bool checkAuthenFromSD (WebServer* server, fs::FS &fs) {
  strcpy(www_password, "");
  strcpy(www_username, "");
  String uuid = extractUuidFromRequest(server->header("Authorization"));
  if (uuid.isEmpty()) {
    return false;
  } else {
    strcpy(www_username, uuid.c_str());
    char fileContent[1024] = "";
    char path[] = "/auth/";
    getJsonFromFile(fs, path, fileContent);
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, fileContent);

    // Check for parsing errors
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    PasswordStruct password;
    password.value =  doc["passwords"][0]["value"];
    password.date = doc["passwords"][0]["date"];
    Serial.println(password.value);

    if (password.value != NULL && strlen(password.value) != 0) {
      strcpy(www_password, password.value);
    } else {
      return false;
    }
  }

  return server->authenticate(&check_bearer_or_auth);
}

bool writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);
  bool result = true;

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
    result = false;
  }
  file.close();
  return result;
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

void getJsonFromFile(fs::FS &fs, char *path, char* fileContent) {
  strcat(path, www_username);
  strcat(path, ".json");
  readFile(SD, path, fileContent);
};

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
  Rtc.Begin();
  IPAddress myIP = WiFi.softAPIP();
  ArduinoOTA.begin();

  // Convert token to a convenient binary representation.
  size_t len = HEXBuilder::hex2bytes(_bearer, sizeof(_bearer), secret_token_hex);
  if (len != 20) {
    Serial.println("Bearer token does not look like a valid SHA1 hex string ?!");
  }

  server.on("/config", HTTP_GET, []() {
    bool authed = checkAuthenFromSD(&server, SD);
    if (!authed) {
      return server.send(401, "text/plain", UNAUTHORIZED);
    }
    char fileContent[1024] = "";
    char path[] = "/config/";
    getJsonFromFile(SD, path, fileContent);
    String response = fileContent;
    server.send(200, "json/application", response);
  });

    //Get present time
  server.on("/time", HTTP_GET, []() {
    DynamicJsonDocument doc(100);

    RtcDateTime now = Rtc.GetDateTime();
    if (now.IsValid()) {
      char str[25];
      sprintf(str, "%d-%d-%dT%d:%d:%d.000Z",       
              now.Year(), 
              now.Month(), 
              now.Day(),    
              now.Hour(),  
              now.Minute(),
              now.Second() 
            );
      doc["time"] = str;
    } else {
      doc["time"] = "1970-01-01T00:00:00.000Z";
    }
    char time_json[100];
    serializeJson(doc, time_json);
    server.send(200, "json/application", time_json);
  });

  server.on("/time", HTTP_POST, [&]() {
    bool authed = checkAuthenFromSD(&server, SD);
    if (!authed) {
      return server.send(401, "text/plain", UNAUTHORIZED);
    }

    String requestBody = server.arg("plain");
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, requestBody);

    // Check for parsing errors
    if (error) {
      Serial.print(F("deserializeJson() request body failed: "));
      Serial.println(error.f_str());
      return server.send(400, "text/plain", BAD_REQUEST);
    }
    // doc = {"year": 2025, "month": 1, "day" : 3, "hour": 2, "minute": 34, "second": 0}
    RtcDateTime newTime(doc["year"], doc["month"], doc["day"], doc["hour"], doc["minute"], doc["second"]);
    Rtc.SetDateTime(newTime);

    if (Rtc.GetIsWriteProtected()) {
      Rtc.SetIsWriteProtected(false);
    }
    if (!Rtc.GetIsRunning()) {
      Rtc.SetIsRunning(true);
    }
    RtcDateTime now = Rtc.GetDateTime();
    if(now.IsValid()){
      return server.send(200, "text/plain", "ok");
    }
      return server.send(400, "text/plain", BAD_REQUEST);
    
  });

  server.on("/cycles", HTTP_POST, []() {
    bool authed = checkAuthenFromSD(&server, SD);
    if (!authed) {
      return server.send(401, "text/plain", UNAUTHORIZED);
    }
    if (server.method() == HTTP_POST) {
      String requestBody = server.arg("plain");
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, requestBody);

      // Check for parsing errors
      if (error) {
        Serial.print(F("deserializeJson() request body failed: "));
        Serial.println(error.f_str());
        return server.send(400, "text/plain", BAD_REQUEST);
      }

      char fileContent[1024] = "";
      char path[] = "/config/";
      getJsonFromFile(SD, path, fileContent);
      DynamicJsonDocument configDoc(1024);
      error = deserializeJson(configDoc, fileContent);
      if (error) {
        Serial.print(F("deserializeJson() config failed: "));
        Serial.println(error.f_str());
        return server.send(500, "text/plain", INTERNAL_SERVER_ERROR);
      }
      int currentCounter = configDoc["cycle_counter"];
      int nextCounter = currentCounter + 1;
      configDoc["cycle_counter"] = nextCounter;
      JsonArray cycles = configDoc["cycles"].as<JsonArray>();
      JsonObject newCycle = cycles.createNestedObject();
      newCycle["id"] = nextCounter;
      newCycle["start"] = doc["start"];
      newCycle["end"] = doc["end"];
      JsonArray day = newCycle.createNestedArray("day");
      day.set(doc["day"].as<JsonArray>());
      newCycle["fan_enable"] = doc["fan_enable"];
      newCycle["fan_delay"] = doc["fan_delay"];
      char updatedContent[1024];
      serializeJson(configDoc, updatedContent);
      bool success = writeFile(SD, path, updatedContent);
      if (!success) {
        return server.send(500, "text/plain", INTERNAL_SERVER_ERROR);
      }
      return server.send(200, "text/plain", "ok");
    }
  });

  server.on("/", HTTP_GET, []()
    {
      bool authed = checkAuthenFromSD(&server, SD);
      if (!authed) {
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
