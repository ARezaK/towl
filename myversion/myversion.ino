#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <Time.h>
#include <Base32.h>
#include <ESP8266WiFi.h>
/*
   This sample sket2ch demonstrates the normal use of a TinyGPS++ (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
static const int RXPin = 12, TXPin = 13;
static const uint32_t GPSBaud = 9600;

// TSTORE_SZ = max number of telemetry entries to backlog
#define TSTORE_SZ 100  // VANDER - WHATS MAX SIZE? I ThINK each entry is 28 bytes


#define DEVICE_ID "a01"
#define SUBDOMAIN "towl.areza.win"


// The TinyGPS++ object
Base32 base32;
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

// Function prototypes:
void parseGPS(void);
void setGPSTime(void);
struct telem * getTelem(void);
uint16_t findSlot(uint8_t);
void storeTelem(struct telem *);
uint8_t sendStoredTelem(void);
int connectAP(void);
uint8_t sendDNSTelem(struct telem *);
void setup(void);
void loop(void);

struct telem {
  uint32_t tstamp;
  double lat;
  double lon;
  double spd;
};  //X bytes total


struct telem tstore[TSTORE_SZ];
uint8_t query_id = 0;
uint32_t last_rec = 0; // Last record time
uint32_t last_rep = 0; // Last report time

void setup()
{
  Serial.begin(9600);
  ss.begin(GPSBaud);
  Serial.println(F("DeviceExample.ino"));
  Serial.println();
}

void loop()
{
  telem *currentpos;
  uint8_t res;
  // This sketch displays information every time a new sentence is correctly encoded.
  res = 0;
  while (ss.available() > 0)
    if (gps.encode(ss.read()))
      if (gps.location.isValid()) {
        displayInfo();
        currentpos = getTelem();
        // store it
        storeTelem(currentpos);

        // if connect AP is good send out some telemtery
        if (connectAP() == 1) Serial.println("enabling this causes crash");//sendStoredTelem();

      }
      else {
        Serial.println(F("INVALID Location AND OR TIME"));
      }


  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
  }
}

uint16_t findSlot() {
  // Find a memory slot that is empty or at a higher time
  // resolution than the current object.
  uint16_t i;
  for (i = 0; i < TSTORE_SZ-1; i++) {
    if (tstore[i].tstamp == 0) {
      tstore[i + 1].tstamp = 0; // make the buffer slot above this one empty
      return i;  // Return the empty slot
    }
  }
  tstore[0].tstamp = 0; //reached the end so make the beggining of the buffer empty
  return i;
}

void storeTelem(struct telem *tdata) {
  uint16_t slot;

  slot = findSlot();
  if (slot == TSTORE_SZ) return; // Buffer full at current res.

  tstore[slot].tstamp = tdata->tstamp;
  tstore[slot].lat = tdata->lat;
  tstore[slot].lon = tdata->lon;
  tstore[slot].spd = tdata->spd;

  Serial.print("Telem stored in slot ");
  Serial.println(slot);
  return;
}

uint8_t sendStoredTelem() {
  uint8_t i, res = 0;
  for(i=0; i < TSTORE_SZ; i++) {
    if (tstore[i].tstamp != 0) {
      res = sendDNSTelem(&tstore[i]);
      if (res == 1) tstore[i].tstamp = 0;
    }
  }
  Serial.println("Sent stored Telem");
  return 0;
}

uint8_t sendDNSTelem(struct telem *tdata) {
  IPAddress qresponse = {0,0,0,0};
  char query[127];
  unsigned int outlen;
  byte *b32string;

  memset(query, '\0', sizeof(query));
  outlen = base32.toBase32((byte*)tdata, sizeof(struct telem), b32string, false);
  strcat(query, "S-");
  strncpy(query+2, (char*)b32string, outlen);
  strcat(query, ".");
  strcat(query, DEVICE_ID);
  strcat(query, ".");
  strcat(query, SUBDOMAIN);
  free(b32string);  // Got malloc()'d inside base32.toBase32()

  WiFi.hostByName(query, qresponse);
  if (qresponse[0] == 10 && qresponse[3] == tdata->tstamp) {
    Serial.println("Telemetry ACK");
    return 1;
  }
  Serial.println("No confirm.");
  return 0;
}


int connectAP() {
  uint16_t numNets;
  uint16_t numOpen = 0;
  int16_t bestcandidate[2] = { -1, -1};
  int16_t bestsignal[2] = { -255, -255};
  int wstatus = WL_IDLE_STATUS;
  char wSSID[64];
  wSSID[63] = 0;

  Serial.println("Scanning WiFi");
  numNets = WiFi.scanNetworks();
  for (uint16_t thisNet = 0; thisNet < numNets; thisNet++) {
    if (WiFi.encryptionType(thisNet) == ENC_TYPE_NONE) {
      numOpen++;
      Serial.print("OPEN SSID: ");
      Serial.print(WiFi.SSID(thisNet));
      Serial.print(" RSSI: ");
      Serial.println(WiFi.RSSI(thisNet));
      for (uint8 i = 0; i < 2; i++) {
        if (WiFi.RSSI(thisNet) > bestsignal[i]) {
          bestcandidate[i] = thisNet;
          bestsignal[i] = WiFi.RSSI(thisNet);
          break;
        }
      }
    }
  }

  if (bestcandidate[0] > -1) {
    uint8_t choice = 0;
    if (bestcandidate[1] > -1) choice = random(2);
    strncpy(wSSID, WiFi.SSID(bestcandidate[choice]).c_str(), sizeof(wSSID) - 1);
    Serial.print("Attempting connect via ");
    Serial.print(wSSID);
    WiFi.disconnect();
    WiFi.persistent(false);

    wstatus = WiFi.begin(wSSID, NULL, 0, NULL, true);

    for (uint8_t i = 0; i < 65; i++) {
      if (wstatus == WL_CONNECTED) {
        Serial.print(". connected. ");
        Serial.println(i * 100);
        return 1;
      }
      if (wstatus == WL_CONNECT_FAILED) {
        Serial.println(". connect failed.");
        return 0;
      }

      wstatus = WiFi.status();
    }
    Serial.println(". timeout.");
  }
  return 0;
}

struct telem *getTelem() {
  struct telem *tdata = new struct telem;


  tdata->tstamp = now();
  tdata->lat = gps.location.lat();
  tdata->lon = gps.location.lng();
  tdata->spd = gps.speed.mph();

  return tdata;
}

void displayInfo()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}
