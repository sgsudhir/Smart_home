#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <string.h>

//By memory_Manager
#define MEM_SIGN 1
#define MEM_SSID 2
#define MEM_PSSWD 3
#define MEM_TIMER 4
#define MEM_IO_STATE 5


/* memory_Manager reads and writes into EEPROM
 * data = "" for reading, data != "" for writing
 * type - Operation type
 * index - Incase present like MEM_TIMER
*/
String memory_Manager (String data, int type, int index) {
  int start_bit = 0;
  int stop_bit = 0;
  int len = 0;
  String response = "";
  int arr[len];
  int cur = 0;
  switch (type) {
    case 1: start_bit = 0; stop_bit = 4; len = 5; break;
    case 2: start_bit = 11; stop_bit = 50; len = 40; break;
    case 3: start_bit = 51; stop_bit = 80; len = 30; break;
    case 4: if (index > 10 || index < 0) break; start_bit = index * 15 + 101; stop_bit = start_bit + 14; len = 15; break;
    case 5: start_bit = 291; stop_bit = 300; len = 10; break;
  }
  if (data == "") {
    //Read Command
    cur = 0;
    for (int i = start_bit; i <= stop_bit; i++) {
      char ch = (char) EEPROM.read (i);
      if (ch == '`') break;
      response += ch;
    }
  } else {
    //Write Command
    char char_data[len];
    memset (char_data, 0, sizeof (char_data));
    strcpy (char_data, data.c_str());
    cur = 0;
    for (int i = start_bit; i <= stop_bit; i++)
      EEPROM.write (start_bit + cur, (int) char_data[cur++]);
  }
  if (data != "") EEPROM.commit();
  return response;  
}



void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
}

void loop() {
  
}



