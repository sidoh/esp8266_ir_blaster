#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <IrHelper.h>
#include <Settings.h>
#include <ArduinoJson.h>
#include <StringStream.h>
#include <fs.h>
#include <HmacHelpers.h>
#include <NtpClientLib.h>

ESP8266WebServer server(80);

WiFiManager wifiManager;
WiFiClient client;
IRrecv irrecv(IR_RECV_GPIO);
IRsend irsend(IR_SEND_GPIO);
decode_results results;
Settings settings;

bool isAuthenticated() {
  String username = settings.adminUsername;
  String password = settings.adminPassword;
  
  if (username && password && username.length() > 0 && password.length() > 0) {
    if (!server.authenticate(username.c_str(), password.c_str())) {
      server.requestAuthentication();
      return false;
    }
  }
  
  return true;
}

void setup() {
  Serial.begin(115200);
  NTP.begin();
  wifiManager.autoConnect();
  if (! SPIFFS.begin()) {
    Serial.println("Failed to initialize SPFFS");
  }
  Settings::load(settings);
  
  const char * headers[] = {
    "X-Signature",
    "X-Signature-Timestamp"
  };
  server.collectHeaders(headers, sizeof(headers)/sizeof(char*));
  
  server.on("/", HTTP_GET, []() {
    if (isAuthenticated()) {
      String body = "<html>";
      body += "<body>";
      body += "<h1>ESP8266 - ";
      body += String(ESP.getChipId(), HEX);
      body += "</h1>";
      body += "<h3>Settings</h3>";
      
      body += "<form method='post' action='/settings'>";
      body += "<div>";
      body += "<textarea name='settings' rows='20' cols='80' style='font-family: courier new, courier, mono;'>";
      body += settings.toJson();
      body += "</textarea>";
      body += "</div>";
      body += "<p><input type='submit'/></p></form>";
      
      body += "<h3>Update Firmware</h3>";
      body += "<form method='post' action='/update' enctype='multipart/form-data'>";
      body += "<input type='file' name='update'><input type='submit' value='Update'></form>";
      
      body += "</body></html>";
      
      server.send(
        200,
        "text/html",
        body
      );
    }
  });
  
  server.on("/settings", HTTP_POST, []() {
    if (isAuthenticated()) {
      Settings::deserialize(settings, server.arg("settings"));
      settings.save();
      
      server.sendHeader("Location", "/");
      server.send(302, "text/plain");
      
      ESP.restart();
    }
  });
  
  server.on("/ir", HTTP_POST, []() {
    String body = server.arg("plain");
    bool validationPassed = true;
    
    if (settings.hmacSecret && settings.hmacSecret.length() > 0) {
      time_t timestamp = server.header("X-Signature-Timestamp").toInt();
      String signature = server.header("X-Signature");
      String computedSignature = requestSignature(
        settings.hmacSecret,
        "/ir",
        body,
        timestamp
      );
      
      if (abs(timestamp - NTP.getTime()) > 20) {
        validationPassed = false;
        server.send(412, "text/plain", "Timestamp expired.");
      } else if (signature != computedSignature) {
        validationPassed = false;
        server.send(412, "text/plain", "Invalid signature. last sync = " + String(NTP.getLastNTPSync()));
      }
    }
    
    if (validationPassed) {
      StaticJsonBuffer<SETTINGS_SIZE> buffer;
      JsonObject& request = buffer.parse(server.arg("plain"));
      
      if (!request.size()) {
        server.send(400, "text/plain", "Invalid JSON: " + server.arg("plain"));
      } else {
        if (request["type"] == "sony") {
          for (int i = 0; i < 3; i++) {
            irsend.sendSony(request["data"], request["num_bits"]);
            delay(40);
            yield();
          }
      
          server.send(200, "text/plain", "true");
        } else if (request["type"] == "raw") {
          JsonArray& data = request["data"];
          unsigned int * buffer = new unsigned int[data.size()];
          data.copyTo(buffer, data.size());
          unsigned int pwmFrequency = request["pwm_frequency"];
          
          irsend.sendRaw(buffer, data.size(), pwmFrequency);
          
          server.send(200, "text/plain", "true");
        }
      }
    }
  });
  
  server.on("/ir", HTTP_GET, []() {
    while (!irrecv.decode(&results)) {
      yield();
    }
    
    String buffer = "";
    StringStream stream(buffer);
    
    dumpInfo(&results, stream);
    dumpRaw(&results, stream);
    dumpCode(&results, stream);
    
    server.send(200, "text/plain", buffer);
    irrecv.resume();
  });
  
  server.on("/update", HTTP_POST, [](){
    if (isAuthenticated()) {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    }
  },[](){
    if (isAuthenticated()) {
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
    }
    yield();
  });

  server.begin();
  irsend.begin();
  irrecv.enableIRIn();
}
  
void loop() {
  server.handleClient();
  yield();
}

