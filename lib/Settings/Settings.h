#ifndef _SETTINGS_H_INCLUDED
#define _SETTINGS_H_INCLUDED

#include <Arduino.h>
#include <StringStream.h>

#define IR_SEND_GPIO 16
#define IR_RECV_GPIO 5

#define SETTINGS_SIZE  1024
#define SETTINGS_FILE  "/config.json"
#define SETTINGS_TERMINATOR '\0'

class Settings {
public:
  Settings() :
    hmacSecret(""),
    adminUsername("admin"),
    adminPassword("esp8266")
  { }

  static void deserialize(Settings& settings, String json);
  static void load(Settings& settings);
  
  void save();
  String toJson(const bool prettyPrint = true);
  void serialize(Stream& stream, const bool prettyPrint = false);
  
  String hmacSecret;
  String adminUsername;
  String adminPassword;
};

#endif 