#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <IrHelper.h>
#include <Settings.h>
#include <ArduinoJson.h>
#include <StringStream.h>

ESP8266WebServer server(80);

WiFiManager wifiManager;
WiFiClient client;
IRrecv irrecv(IR_RECV_GPIO);
IRsend irsend(IR_SEND_GPIO);
decode_results results;

void setup() {
  Serial.begin(115200);
  wifiManager.autoConnect();
  
  server.on("/", HTTP_GET, [](){
    String body = "<html><head><title>IR Server</title></head>";
    body += "<body>";
    body += "<form method='post' action='/ir'>";
    body += "<h5>Type</h5>";
    body += "<div><input type='radio' name='type' value='sony' /> Sony</div>";
    body += "<div><input type='radio' name='type' value='raw' /> Raw</div>";
    body += "<h5>Code</h5>";
    body += "<textarea rows='3' cols='80' style='font-family: courier new, courier, mono;'></textarea>";
    body += "<input type='submit' />";
    body += "</form></body></html>";
    
    server.send(200, "text/html", body);
  });
  
  server.on("/ir", HTTP_POST, []() {
    StaticJsonBuffer<1024> buffer;
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
    
        server.send(200, "text/plain", server.arg("plain"));
      } else if (request["type"] == "raw") {
        JsonArray& data = request["data"];
        unsigned int * buffer = new unsigned int[data.size()];
        data.copyTo(buffer, data.size());
        unsigned int pwmFrequency = request["pwm_frequency"];
        
        irsend.sendRaw(buffer, data.size(), pwmFrequency);
        
        String response = "With data: [";
        for (int i = 0; i < data.size(); i++) {
          response += buffer[i];
          response += ",";
        }
        response += "]\n";
        response += "size: " + String(data.size()) + "\n";
        response += "freq: " + String(pwmFrequency);
        
        server.send(200, "text/plain", response);
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
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
    ESP.restart();
  },[](){
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

