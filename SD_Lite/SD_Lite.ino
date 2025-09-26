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

#define LED_UV 2
#define FAN_UV 0

//connect SD module
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
const char *NOT_FOUND = "Resource not found";

const char *SUCCESS = "{\"status\": \"ok\"}";

const char *PLAIN_TEXT = "text/plain";
const char *JSON_APPLICATION = "json/application";

const char *CONFIG_ROOT_PATH = "/config/";
const char *AUTH_ROOT_PATH = "/auth/";

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
      return new String(params[0]);
    } else {
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
    char fileContent[1024];
    char path[128];
    strcpy(path, AUTH_ROOT_PATH);
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

DynamicJsonDocument readFile2(fs::FS &fs, const char *path, size_t capacity = 10000)
{
  Serial.printf("Reading file: %s\n", path);

  DynamicJsonDocument doc(capacity);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading, returning empty JSON array");
    doc.to<JsonArray>();   // return an empty array if file missing
    return doc;
  }

  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    doc.to<JsonArray>();  // fallback: return empty JSON array
  }

  return doc;
}

int timeToMinutes(const String &timeStr) {
  int h, m;
  sscanf(timeStr.c_str(), "%d:%d", &h, &m); // extract hour & minute
  return h * 60 + m; // convert to minutes
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_UV, OUTPUT);
  digitalWrite(LED_UV, LOW);
  pinMode(FAN_UV, OUTPUT);
  digitalWrite(FAN_UV, LOW);

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
    Serial.println("[/config] - GET - start");
    bool authed = checkAuthenFromSD(&server, SD);
    if (!authed) {
      return server.send(401, PLAIN_TEXT, UNAUTHORIZED);
    }
    char fileContent[1024] = "";
    char path[128];
    strcpy(path, CONFIG_ROOT_PATH);
    getJsonFromFile(SD, path, fileContent);
    Serial.println("[/config] - GET - return");
    return server.send(200, JSON_APPLICATION, fileContent);
  });

    //Get present time
  server.on("/time", HTTP_GET, []() {
    DynamicJsonDocument doc(100);
    if (!Rtc.GetIsRunning()) {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
    }
    if (Rtc.GetIsWriteProtected()) {
      Rtc.SetIsWriteProtected(false);
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (now.IsValid()) {
      char str[25];
      sprintf(str, "%04d-%02d-%02dT%02d:%02d:%02d.000Z",       
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
      return server.send(401, PLAIN_TEXT, UNAUTHORIZED);
    }
    String requestBody = server.arg("plain");
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, requestBody);
    // Check for parsing errors
    if (error) {
      Serial.print(F("deserializeJson() request body failed: "));
      Serial.println(error.f_str());
      return server.send(400, PLAIN_TEXT, BAD_REQUEST);
    }
    if (!Rtc.GetIsRunning()) {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
    }
    if (Rtc.GetIsWriteProtected()) {
      Rtc.SetIsWriteProtected(false);
    }
    // doc = {"year": 2025, "month": 1, "day" : 3, "hour": 2, "minute": 34, "second": 0}
    RtcDateTime newTime(
      doc["year"], doc["month"], doc["day"], doc["hour"], doc["minute"], doc["second"]
    );
    Rtc.SetDateTime(newTime);
    RtcDateTime now = Rtc.GetDateTime();
    if(now.IsValid()){
      return server.send(200, JSON_APPLICATION, SUCCESS);
    }
    return server.send(400, PLAIN_TEXT, BAD_REQUEST);
  });

  server.on("/cycles", HTTP_POST, []() {
    bool authed = checkAuthenFromSD(&server, SD);
    if (!authed) {
      return server.send(401, PLAIN_TEXT, UNAUTHORIZED);
    }
    String requestBody = server.arg("plain");
    if (requestBody == NULL || requestBody.isEmpty()) {
      server.send(400, PLAIN_TEXT, BAD_REQUEST);
    }
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, requestBody);
    if (error) {
      Serial.print(F("deserializeJson() request body failed: "));
      Serial.println(error.f_str());
      return server.send(400, PLAIN_TEXT, BAD_REQUEST);
    }
    char fileContent[1024] = "";
    char path[128];
    strcpy(path, CONFIG_ROOT_PATH);
    getJsonFromFile(SD, path, fileContent);
    StaticJsonDocument<1024> configDoc;
    error = deserializeJson(configDoc, fileContent);
    if (error) {
      Serial.print(F("deserializeJson() config failed: "));
      Serial.println(error.f_str());
      return server.send(500, PLAIN_TEXT, INTERNAL_SERVER_ERROR);
    }
    int currentCounter = configDoc["cycle_counter"];
    int nextCounter = currentCounter + 1;
    configDoc["cycle_counter"] = nextCounter;
    JsonArray cycles = configDoc["cycles"].as<JsonArray>();
    if (cycles.size() < 5) {
      JsonObject newCycle = cycles.createNestedObject();
      newCycle["id"] = nextCounter;
      newCycle["status"] = doc["status"];
      newCycle["start"] = doc["start"];
      newCycle["end"] = doc["end"];
      JsonArray newDay = newCycle.createNestedArray("day");
      newDay.set(doc["day"].as<JsonArray>());
      newCycle["fan_enable"] = doc["fan_enable"];
      newCycle["fan_delay"] = doc["fan_delay"];
      serializeJson(configDoc, fileContent);
      bool success = writeFile(SD, path, fileContent);
      if (!success) {
        return server.send(500, PLAIN_TEXT, INTERNAL_SERVER_ERROR);
      }
      serializeJson(newCycle, fileContent);
      return server.send(200, JSON_APPLICATION, fileContent);
    }
    return server.send(400, PLAIN_TEXT, BAD_REQUEST);

  });

  server.on("/cycles", HTTP_PUT, []() {
    bool authed = checkAuthenFromSD(&server, SD);
    if (!authed) {
      return server.send(401, PLAIN_TEXT, UNAUTHORIZED);
    }
    String requestBody = server.arg("plain");
    if (requestBody == NULL || requestBody.isEmpty()) {
      server.send(400, PLAIN_TEXT, BAD_REQUEST);
    }
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, requestBody);
    if (error || !doc["id"]) {
      if (error) {
        Serial.print(F("deserializeJson() request body failed: "));
        Serial.println(error.f_str());
      }
      return server.send(400, PLAIN_TEXT, BAD_REQUEST);
    }
    char fileContent[1024] = "";
    char path[128];
    strcpy(path, CONFIG_ROOT_PATH);
    getJsonFromFile(SD, path, fileContent);
    StaticJsonDocument<1024> configDoc;
    error = deserializeJson(configDoc, fileContent);
    if (error) {
      Serial.print(F("deserializeJson() config failed: "));
      Serial.println(error.f_str());
      return server.send(500, PLAIN_TEXT, INTERNAL_SERVER_ERROR);
    }
    JsonArray cycles = configDoc["cycles"].as<JsonArray>();
    for (int i = 0; i < cycles.size(); i++) {
      JsonObject cycle = cycles[i].as<JsonObject>();
      if (cycle["id"] != doc["id"]) {
        continue;
      } else {
        cycle["status"] = doc["status"];
        cycle["start"] = doc["start"];
        cycle["end"] = doc["end"];
        JsonArray updatedDay = cycle["day"].as<JsonArray>();
        updatedDay.set(doc["day"].as<JsonArray>());
        cycle["fan_enable"] = doc["fan_enable"];
        cycle["fan_delay"] = doc["fan_delay"];
        serializeJson(configDoc, fileContent);
        bool success = writeFile(SD, path, fileContent);
        if (!success) {
          return server.send(500, PLAIN_TEXT, INTERNAL_SERVER_ERROR);
        }
        serializeJson(cycle, fileContent);
        return server.send(200, JSON_APPLICATION, fileContent);
      }
    }
    return server.send(404, PLAIN_TEXT, NOT_FOUND);
  });

  server.on("/cycles", HTTP_DELETE, []() {
    bool authed = checkAuthenFromSD(&server, SD);
    if (!authed) {
      return server.send(401, PLAIN_TEXT, UNAUTHORIZED);
    }
    String requestId = server.arg("id");
    if (requestId == NULL || requestId.isEmpty()) {
      server.send(400, PLAIN_TEXT, BAD_REQUEST);
    }
    std::string idString = requestId.c_str(); 
    char fileContent[1024] = "";
    char path[128];
    strcpy(path, CONFIG_ROOT_PATH);
    getJsonFromFile(SD, path, fileContent);
    StaticJsonDocument<1024> configDoc;
    DeserializationError error = deserializeJson(configDoc, fileContent);
    if (error) {
      Serial.print(F("deserializeJson() config failed: "));
      Serial.println(error.f_str());
      return server.send(500, PLAIN_TEXT, INTERNAL_SERVER_ERROR);
    }
    JsonArray cycles = configDoc["cycles"].as<JsonArray>();
    for (int i = 0; i < cycles.size(); i++) {
      JsonObject cycle = cycles[i].as<JsonObject>();
      if (cycle["id"] != std::__cxx11::stoi(idString)) {
        continue;
      } else {
        cycles.remove(i);
        serializeJson(configDoc, fileContent);
        bool success = writeFile(SD, path, fileContent);
        if (!success) {
          return server.send(500, PLAIN_TEXT, INTERNAL_SERVER_ERROR);
        }
        char response[32];
        sprintf(response, "{\"id\":%s}",requestId.c_str());
        return server.send(200, JSON_APPLICATION ,response);
      }
    }
    return server.send(404, PLAIN_TEXT, NOT_FOUND);
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
          server.send(404, PLAIN_TEXT, NOT_FOUND);
      }
    });

  server.serveStatic("/", SD, "/ui/");
  server.begin();

  Serial.print("Open http://");
  Serial.print(myIP);
  Serial.println("/ in your browser to see it working");
}

int check = 0;             // 0 = searching, 1 = LED cycle running, 2 = FAN delay running
int lastSelectedId = -1;   // remember which cycle is active
int time_end = 0;          // LED off time (minutes)
int fan_end = 0;           // FAN off time (minutes)

void checkCycles() {
  RtcDateTime now = Rtc.GetDateTime();
  int nowMinutes = now.Hour() * 60 + now.Minute();
  int weekday = now.DayOfWeek();

  // --- Case 1: LED phase ---
  if (check == 1) {
    Serial.println("Check cycle = 1, LED phase");
    if (nowMinutes >= time_end) {
      // LED finished
      digitalWrite(LED_UV, LOW);
      Serial.println("LED OFF");

      // Read cycle settings
      DynamicJsonDocument doc = readFile2(SD, "/config/admin.json", 7000);
      JsonArray cycles = doc["cycles"];

      for (JsonObject cycle : cycles) {
        if (cycle["id"].as<int>() == lastSelectedId) {
          int fanEnable = cycle["fan_enable"].as<int>();
          int fanDelay  = cycle["fan_delay"].as<int>();

          if (fanEnable == 1) {
            fan_end = time_end + fanDelay;
            digitalWrite(FAN_UV, HIGH);
            Serial.printf("FAN ON (delay %d min)\n", fanDelay);
            check = 2; // go to FAN phase
          } else {
            Serial.println("FAN disabled for this cycle");
            digitalWrite(FAN_UV, LOW);
            check = 0; // back to searching
            lastSelectedId = -1;
          }
          break; // stop loop once found
        }
      }
    }
    return;
  }

  // --- Case 2: FAN phase ---
  if (check == 2) {
    Serial.println("Check cycle = 2, FAN phase");
    if (nowMinutes >= fan_end) {
      digitalWrite(FAN_UV, LOW);
      Serial.println("FAN OFF (delay finished)");
      check = 0;            // back to searching
      lastSelectedId = -1;
    }
    return;
  }

  // --- Case 3: searching for cycle (check == 0) ---
  if(check == 0){
    Serial.println("Check cycle = 0, check phase");
    DynamicJsonDocument doc = readFile2(SD, "/config/admin.json", 7000);
    if (doc.isNull()){
      Serial.println("Doc is NULL");
      return;
    } 

    JsonArray cycles = doc["cycles"];
    if (cycles.isNull()){
      Serial.println("Cycle is NULL");
      return;
    } 

    int selectedId = -1;
    JsonObject selectedCycle;
    
    for (JsonObject cycle : cycles) {
      int startM = timeToMinutes(cycle["start"].as<String>());
      int endM   = timeToMinutes(cycle["end"].as<String>());
      int status = cycle["status"].as<int>();

      if (status == 1 && cycle["day"][weekday] == 1 && nowMinutes >= startM && nowMinutes <= endM) {
        int cid = cycle["id"].as<int>();
        if (selectedId == -1 || cid < selectedId) {
          selectedId = cid;
          selectedCycle = cycle;
        }
      }
    }

    if (selectedId != -1) {
      Serial.printf("Cycle %d started → LED ON\n", selectedId);
      digitalWrite(LED_UV, HIGH);
      digitalWrite(FAN_UV, LOW); // fan waits until LED ends
      time_end = timeToMinutes(selectedCycle["end"].as<String>());
      check = 1;                 // LED phase
      lastSelectedId = selectedId;
    }
  }
  
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
  delay(2); // allow the cpu to switch to other tasks
  checkCycles();
  delay(2);
}
