#ifndef _IR_HELPER_H
#define _IR_HELPER_H

#include <Arduino.h>
#include <IRremoteESP8266.h>

void ircode(decode_results *results, Stream& client);
void encoding(decode_results *results, Stream& client);
void dumpInfo(decode_results *results, Stream& client);
void dumpRaw(decode_results *results, Stream& client);
void dumpCode(decode_results *results, Stream& client);

#endif