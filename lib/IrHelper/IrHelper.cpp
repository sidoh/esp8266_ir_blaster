#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IrHelper.h>

void ircode(decode_results *results, Stream& client) {
  // Panasonic has an Address
  if (results->decode_type == PANASONIC) {
    client.print(results->panasonicAddress, HEX);
    client.print(":");
  }

  // Print Code
  client.print(results->value, HEX);
}

void encoding(decode_results *results, Stream& client) {
  switch (results->decode_type) {
    default:
    case UNKNOWN:      client.print("UNKNOWN");       break ;
    case NEC:          client.print("NEC");           break ;
    case SONY:         client.print("SONY");          break ;
    case RC5:          client.print("RC5");           break ;
    case RC6:          client.print("RC6");           break ;
    case DISH:         client.print("DISH");          break ;
    case SHARP:        client.print("SHARP");         break ;
    case JVC:          client.print("JVC");           break ;
    case SANYO:        client.print("SANYO");         break ;
    case MITSUBISHI:   client.print("MITSUBISHI");    break ;
    case SAMSUNG:      client.print("SAMSUNG");       break ;
    case LG:           client.print("LG");            break ;
    case WHYNTER:      client.print("WHYNTER");       break ;
    case AIWA_RC_T501: client.print("AIWA_RC_T501");  break ;
    case PANASONIC:    client.print("PANASONIC");     break ;
  }
}

void dumpInfo(decode_results *results, Stream& client) {
  // Show Encoding standard
  client.print("Encoding  : ");
  encoding(results, client);
  client.println("");

  // Show Code & length
  client.print("Code      : ");
  ircode(results, client);
  client.print(" (");
  client.print(results->bits, DEC);
  client.println(" bits)");
}

void dumpRaw(decode_results *results, Stream& client) {
  // Print Raw data
  client.print("Timing[");
  client.print(results->rawlen-1, DEC);
  client.println("]: ");

  for (int i = 1;  i < results->rawlen;  i++) {
    unsigned long  x = results->rawbuf[i] * USECPERTICK;
    if (!(i & 1)) {  // even
      client.print("-");
      if (x < 1000)  client.print(" ") ;
      if (x < 100)   client.print(" ") ;
      client.print(x, DEC);
    } else {  // odd
      client.print("     ");
      client.print("+");
      if (x < 1000)  client.print(" ") ;
      if (x < 100)   client.print(" ") ;
      client.print(x, DEC);
      if (i < results->rawlen-1) client.print(", "); //',' not needed for last one
    }
    if (!(i % 8))  client.println("");
  }
  client.println("");                    // Newline
}

//+=============================================================================
// Dump out the decode_results structure.
//
void dumpCode(decode_results *results, Stream& client) {
  // Start declaration
  client.print("unsigned int  ");          // variable type
  client.print("rawData[");                // array name
  client.print(results->rawlen - 1, DEC);  // array size
  client.print("] = {");                   // Start declaration

  // Dump data
  for (int i = 1;  i < results->rawlen;  i++) {
    client.print(results->rawbuf[i] * USECPERTICK, DEC);
    if ( i < results->rawlen-1 ) client.print(","); // ',' not needed on last one
    if (!(i & 1))  client.print(" ");
  }

  // End declaration
  client.print("};");  // 

  // Comment
  client.print("  // ");
  encoding(results, client);
  client.print(" ");
  ircode(results, client);

  // Newline
  client.println("");

  // Now dump "known" codes
  if (results->decode_type != UNKNOWN) {

    // Some protocols have an address
    if (results->decode_type == PANASONIC) {
      client.print("unsigned int  addr = 0x");
      client.print(results->panasonicAddress, HEX);
      client.println(";");
    }

    // All protocols have data
    client.print("unsigned int  data = 0x");
    client.print(results->value, HEX);
    client.println(";");
  }
}