#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <IrHelper.h>
#include <Settings.h>

ESP8266WebServer server(80);

WiFiManager wifiManager;
WiFiClient client;
IRrecv irrecv(IR_RECV_GPIO);
IRsend irsend(IR_SEND_GPIO);
decode_results results;

void setup() {
  Serial.begin(115200);
  wifiManager.autoConnect();
  
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
  
  // unsigned int  rawData[46] = {1850,700, 1800,1950, 600,1900, 600,700, 1850,650, 1900,650, 1850,1950, 550,700, 1850,1950, 600,1950, 600,700, 1850,700, 1850,700, 1850,1900, 600,700, 1850,650, 1850,1950, 600,1950, 600,700, 1850,700, 1850,650, 1900,1950, 600,700 };  // UNKNOWN 38949E4C
  
  bool send = false;
  bool recv = true;
  
void loop() {
  server.handleClient();
  
  if (recv) {
    if (irrecv.decode(&results)) {
      dumpInfo(&results, client);           // Output the results
      dumpRaw(&results, client);            // Output the results in RAW format
      dumpCode(&results, client);           // Output the results as source code
      client.println("");           // Blank line between entries
      irrecv.resume();
      irrecv.disableIRIn();
      recv = false;
      send = true;
    }
  }
  
  if (send) {
    client.println("Trying to re-send packet of length: " + String(results.rawlen) + " in...");
    
    for (int i = 3; i >= 1; i--) {
      client.print(i);
      client.println("...");
      delay(1000);
    }
    
    client.println("Sending!");
  
    unsigned int * rawData = new unsigned int[results.rawlen];
    
    for (int i = 0; i < results.rawlen; i++) {
      rawData[i] = results.rawbuf[i]*USECPERTICK;
      client.print(rawData[i]);
      client.print(" ");
    }
    
    client.println();
    client.println("Data constructed");
    
    irsend.sendRaw(rawData, results.rawlen, 38);
    // delay(40);
    // irsend.sendRaw(rawData, results.rawlen-1, 38);
    
    client.println("Sent!");
    
    delete rawData;
    
    send = false;
    recv = true;
    irrecv.enableIRIn();
  }
  
  // delay(1000);
  // client.println("Trying to send sony...");
  // irsend.sendSony(0xA90,12);
  // delay(40);
  // irsend.sendSony(0xA90,12);
  // delay(40);
  // irsend.sendSony(0xA90,12);
    
  client.flush();
  server.handleClient();
  yield();
}

