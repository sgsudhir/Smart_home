#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <string.h>
#include <ESP8266mDNS.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//By memory_Manager
#define MEM_SIGN 1
#define MEM_SSID 2
#define MEM_PSSWD 3
#define MEM_TIMER 4
#define MEM_IO_STATE 5
#define MEM_MQTT_USER 6
#define MEM_MQTT_KEY 7

//By IO_Manager
#define IO_TOGGLE 2
#define IO_ON 1
#define IO_OFF 0 

//By set_Timer
#define FREE_TIMER (String) "999999999999999"

//By NET_Manager
#define NET_FAIL 0
#define NET_STA 1
#define NET_AP 2
#define NET_STA_AP 3
int netType = NET_FAIL;

//By internet_Time
bool timeInit = false; 

//By MQTT_Manager
#define MQTT_SERVER "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_USER "sgsudhir"
#define MQTT_KEY "0e7a8026688646bb83111ad277fbfd49"
WiFiClient mqtt_client;
String memory_Manager (String, int, int);
Adafruit_MQTT_Client mqtt (&mqtt_client, MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_KEY);
Adafruit_MQTT_Publish from_esp = Adafruit_MQTT_Publish (&mqtt, MQTT_USER "/feeds/from_esp_io_1");
Adafruit_MQTT_Subscribe to_esp = Adafruit_MQTT_Subscribe (&mqtt, MQTT_USER "/feeds/to_esp_io_1");


/* 
 *  memory_Manager reads and writes into EEPROM
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
    case 5: start_bit = 291; stop_bit = 298; len = 8; break;
    case 6: start_bit = 301; stop_bit = 330; len = 30; break;
    case 7: start_bit = 331; stop_bit = 370; len = 40; break;
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
    if (type == MEM_SSID || type == MEM_PSSWD || type == MEM_MQTT_USER || type == MEM_MQTT_KEY)
      for (int i = start_bit; i <= stop_bit; i++)
        EEPROM.write (i, '`'); // Initialize SSID or Psswd into EEPROM
    for (int i = start_bit; i <= stop_bit || i <= strlen (char_data); i++)
      EEPROM.write (start_bit + cur, (int) char_data[cur++]);
  }
  if (data != "") EEPROM.commit();
  return response;  
}

/*
 * sys_Reset is used for Factory Reset data
 * bool clean - Also reFormat EEPROM
*/
void sys_Reset (bool clean) {
  if (!clean) { ESP.reset (); return; }
  for (int i = 0; i < 10; i++)
    memory_Manager (FREE_TIMER, MEM_TIMER, i); // Initialize Timer into EEPROM
  for (int i = 11; i <= 100; i++)
    EEPROM.write (i, '`'); // Initialize SSID, Psswd into EEPROM
  for (int i = 300; i <= 360; i++)
    EEPROM.write (i, '`'); // Initialize MQTT User and Key
  EEPROM.commit ();
  memory_Manager ("12345", MEM_SIGN, 0); // Write Signature into EEPROM
  memory_Manager ("00000000", MEM_IO_STATE, 0); // Initialize IO_State into EEPROM
  ESP.reset ();
}

/*
 * NET_Manager activates AP mode or Station mode.
 * returns current activated mode
 * type - NET_STA or NET_AP to activate Station or AP mode respectively 
*/
int NET_Manager (int type) {
  if (type == NET_STA) { // Turn on STA mode
    if ((netType == NET_STA) && (WiFi.reconnect()) && (WiFi.status() == WL_CONNECTED)) return NET_STA;
    WiFi.mode (WIFI_STA);
    netType = NET_FAIL;
    WiFi.begin (memory_Manager ("", MEM_SSID, 0).c_str(), memory_Manager ("", MEM_PSSWD, 0).c_str());
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay (1000);
      if (count++ >= 30) return NET_FAIL;
    }
    netType = NET_STA;
    return NET_STA;
  } else if (type == NET_AP) { // Turn on AP mode
    if (netType == NET_AP) return NET_AP;
    WiFi.disconnect(true);
    WiFi.mode (WIFI_AP);
    netType = NET_FAIL;
    if (WiFi.softAP("IOT_ESP_IO-1", "nodemcuesp8266")) {
      netType = NET_AP; 
      return NET_AP;
    }
    
  }
  return NET_FAIL;
}

/*
 * internet_Time is based on NTP protocol to sync with internet time
 * returns String internet time in success, NULL in failure
*/
String internet_Time () {
  if (!timeInit) {
    configTime(5 * 3600, 30 * 60, "pool.ntp.org","time.nist.gov");
    int wait = 0;
    timeInit = true;
    while(!time(nullptr)){
      delay(1000);
      if (wait >= 30) {
        timeInit = false;
        return "";
      }
      wait++;
    }
  } 
  String hh, mm, ss, dd, mM;
  int yyyy;
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  dd = (String) p_tm->tm_mday;
  mM = (String) p_tm->tm_mon + 1;
  yyyy = (int) p_tm->tm_year + 1900;
  timeInit = false;
  if (yyyy < 2000) return "";
  timeInit = true;
  hh = (String) p_tm->tm_hour;
  mm = (String) p_tm->tm_min; 
  ss = (String) p_tm->tm_sec;
  return (String) (hh + ":" + mm + ":" + ss + ":" + dd);
}

/*
 * MQTT_Manager sends and receives message from/to Adafruit IO MQTT server
 * if int msg = 0 then reads else writes
 * returns NULL in failure and String result on success
*/
String MQTT_Manager (int msg) {
  if (netType != NET_STA) return "";
  if(! mqtt.ping()) mqtt.disconnect();
  if (!mqtt.connected()) {
    int count = 0;
    int8_t ret;
    while ((ret = mqtt.connect()) != 0) {
      mqtt.disconnect();
      if (count >= 20) return "";
      count++;
    }
  }
  if (msg == 0) { // Read from subscription if available
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(5000))) {
      if (subscription == &to_esp) {
        return (String) (char *) to_esp.lastread;
      }
    }
  } else {//Publish changes
    if (!from_esp.publish(msg, 1)) return "";
    return (String) msg;  
  }
  return "";
}

/* 
 *  IO_Manager sets the IO pins and calls memory_Manager to read & write config
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

  mqtt.subscribe(&to_esp);

  
}

int i = 0;
void loop() {
  NET_Manager(NET_STA);
  MQTT_Manager (i);
  i++;
  Serial.println (MQTT_Manager (0));
  Serial.println (memory_Manager ("", MEM_MQTT_USER, 0));
  Serial.println (memory_Manager ("", MEM_MQTT_KEY, 0));
  delay (1000);
}




