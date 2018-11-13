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

//By IO_Manager
#define IO_TOGGLE 2
#define IO_ON 1
#define IO_OFF 0 

//By set_Timer
#define FREE_TIMER (String) "999999999999999"


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
    case 4: if (index > 9 || index < 0) break; start_bit = index * 15 + 101; stop_bit = start_bit + 14; len = 15; break;
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

/* IO_Manager sets the IO pins and calls memory_Manager to read & write config
 * stage - IO_TOGGLE for Toggle, IO_ON for on & IO_OFF for off for String data IOs
*/
bool IO_Manager (String data, int stage) {
  static int ON = 0;
  static int OFF = 1;
  char io_get[8];
  char io_new[8];
  char io_old[8];
  String io_write = "";
  String io_read = memory_Manager ("", MEM_IO_STATE, 0);  //Read previous config from EEPROM
  strcpy (io_get, data.c_str());
  strcpy (io_old, io_read.c_str());
  
  //Generate new config into 'io_new' from 'io_get', 'io_old' & 'stage'
  for (int i = 0; i < 8; i++) {
    if (io_get[i] == '0') io_new[i] = io_old[i];
    else if (io_get[i] == '1')  {
      switch (stage) {
        case IO_TOGGLE: (io_old[i] == '1')? io_new[i] = '0': io_new[i] = '1'; break;
        case IO_ON: io_new[i] = '1'; break;
        case IO_OFF: io_new[i] = '0'; break;  
      }
    }
  }
  for (int i = 0; i< 8; i++) io_write += io_new[i];

  //Set IO Pins
  (io_new[0] == '0')? digitalWrite (D0, OFF): digitalWrite (D0, ON);
  (io_new[1] == '0')? digitalWrite (D1, OFF): digitalWrite (D1, ON);
  (io_new[2] == '0')? digitalWrite (D2, OFF): digitalWrite (D2, ON);
  (io_new[3] == '0')? digitalWrite (D3, OFF): digitalWrite (D3, ON);
  (io_new[4] == '0')? digitalWrite (D5, OFF): digitalWrite (D5, ON);
  (io_new[5] == '0')? digitalWrite (D6, OFF): digitalWrite (D6, ON);
  (io_new[6] == '0')? digitalWrite (D7, OFF): digitalWrite (D7, ON);
  (io_new[7] == '0')? digitalWrite (D8, OFF): digitalWrite (D8, ON);
  
  memory_Manager (io_write, MEM_IO_STATE, 0); //Write new Config into EEPROM
  return false;  
}

/* 
 * timer_Manager sets & deletes timer
 * String data = "" - read, FREE_TIMER - erase timer at index, else set timer
 * returns String timer if String data == "" else returns "".
*/ 
String timer_Manager (String data, int index) {
  if (data == "") return memory_Manager ("", MEM_TIMER, index);
  if (data == FREE_TIMER) return memory_Manager (FREE_TIMER, MEM_TIMER, index);
  
  bool flag = false;
  int i = 0;
  for (i = 0; i < 10; i++) {
    if (FREE_TIMER == memory_Manager ("", MEM_TIMER, i)) {
      flag = true;
      break;
    }
  }
  if (flag) index = i; else index = 9;
  memory_Manager (data, MEM_TIMER, index);
  
  return "";
}


void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
  pinMode (D0, OUTPUT);
  pinMode (D1, OUTPUT);
  pinMode (D2, OUTPUT);
  pinMode (D3, OUTPUT);
  pinMode (D4, OUTPUT);
  pinMode (D5, OUTPUT);
  pinMode (D6, OUTPUT);
  pinMode (D7, OUTPUT);
  pinMode (D8, OUTPUT);

  
}

void loop() {
 
}




