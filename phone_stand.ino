#include <TM1637Display.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include "time.h"
#include <Arduino.h>
#include <ESP.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "SeatedEntry.h"

#define CLK  22 // ESP32 pin GPIO22 connected to CLK
#define DIO  23 // ESP32 pin GPIO23 connected to DIO
#define NEW_DAY  27
#define ADD_DAYS  25
#define BUTTON_PIN 17 // ESP32 pin 17 connected to interrupt

#define DEBUG
#define LOGDATA

const char* ssid = "*****";
const char* password = "****";

const unsigned long ntpInterval = 15 * 60 * 1000;     // 15 minutes in milliseconds
const unsigned long displayInterval = 1000;     // 15 minutes in milliseconds
struct tm currentSeatedStart;
time_t previousSeatedTime;
time_t totalSeatedTime;


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -5 * 3600;                // Eastern Time Zone
const int   daylightOffset_sec = 3600;

TM1637Display display = TM1637Display(CLK, DIO);
Preferences prefs;                              // Non-volatile storage (NVS)

unsigned long numEntries;
unsigned long numcurrentDateEntry;

bool previousRead;
bool buttonVal;

void printLocalTime();
void Task1(void *pvParameters);
int timeToDisplay(time_t x);

Date today;
seatedEntry currentDateEntry;
String tmpID;

bool allSavedData = false;
bool addNewEntries = false;
void IRAM_ATTR handleReadAllSavedData(){
  allSavedData = true;
}
void IRAM_ATTR handleAddingEnrties(){
  addNewEntries = true;
}

void setup() {
  display.clear();
  Serial.begin(115200);

  
  // -------------------------------------------- //
  // Initialize Non-volatile storage
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  prefs.begin("LogData", false);                                  // Creating Namespace
  numEntries = prefs.getULong("numEntries", false);
  
  // -------------------------------------------- //
  
#ifdef DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif

  // Connecting to Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {                         // While Connecting
    delay(500);

#ifdef DEBUG
    Serial.print(".");
#endif
  }
#ifdef LOGDATA
  Serial.println();

  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  if (!SPIFFS.begin(true)) {
    Serial.println("Initialization failed!");
    return;
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);     // Fetch time
  delay(50);                                                    // Wait a little time for connection
  // prefs.putULong("numEntries", 0); 
  // numEntries = 0;

  today = getCurrentDate();                                     // Get Current Date
  if (!findEntryByDate(&currentDateEntry, &today)) {
    Serial.println("No Entry for Current Day");
    appendNewEntry(&currentDateEntry);
  }

  listFiles("/");
  displaySPIFFSSpaceInfo();

  // if (numEntries == 0) {
  // } else { 
  //    Serial.print("Non Zero numEntries: ");
  //    Serial.println(numEntries);
  //    tmpID = "E" + String(numEntries - 1);
  //    Serial.print("Reading Entry: ");
  //    Serial.println(tmpID);
  //    if (prefs.getBytes(tmpID.c_str(), &currentDateEntry, sizeof(seatedEntry)) == sizeof(seatedEntry)){
  //       if (compareToCurrentDay(&currentDateEntry) == false) {
  //         createNewEntry(&currentDateEntry);
  //       } 
  //       printSeatedEntry(&currentDateEntry);
  //    } else {
  //       Serial.println("Failed to fetch data entry");
  //    }
  // }
  
  totalSeatedTime = currentDateEntry.seatedTime;
  previousSeatedTime = currentDateEntry.seatedTime;
  printLocalTime();

  // -------------------------------------------- //
  previousRead = HIGH;

  xTaskCreate(resync_time, "Resync Time", 2000, NULL, 1, NULL);
  xTaskCreate(update_display, "Update Display", 4000, NULL, 1, NULL);

  Serial.print("");

  Serial.println("Configuring display");
  display.clear();
  display.setBrightness(7); // Set the brightness to 7 (0:dimmest, 7:brightest)
  display.clear();
  display.showNumberDecEx(timeToDisplay(totalSeatedTime), 0b11100000, false, 4, 0);

  Serial.println("Configuring GPIOs");
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-down resistor

  pinMode(NEW_DAY, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(NEW_DAY), handleReadAllSavedData, FALLING);

  // pinMode(ADD_DAYS, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(ADD_DAYS), handleAddingEnrties, FALLING);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW)       // this is for inverting the button
    buttonVal = HIGH;
  else
    buttonVal = LOW;
  
  if (buttonVal != previousRead) {
    if (buttonVal == HIGH) {
      previousRead = HIGH;
    } else {
      if(!getLocalTime(&currentSeatedStart)){
        Serial.println("Failed to obtain time for currentSeatedStart");
      }
      previousSeatedTime = mktime(&currentSeatedStart);
      previousRead = LOW;
    }
  }

  if (allSavedData) {
    allSavedData = false;
    printAllEntries();
  }

  // if (addNewEntries) {
  //   if (digitalRead(ADD_DAYS)) {
  //     addNewEntries = false;
  //   } else {
  //     createNewEntry(&currentDateEntry);
  //   }
  // }
  
  // Small delay to prevent overwhelming the serial output
  delay(50);
}

void resync_time(void *pvParameters) {
  while (1) {
    Serial.println("Task 1 Resynchronizing Time...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    vTaskDelay(ntpInterval / portTICK_PERIOD_MS);
  }
}

void update_display(void *pvParameters) {
  struct tm timeinfo;
  time_t current = 0;
  while (1) {
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
    } else{
      if (compareToCurrentDay(&currentDateEntry) == false) {
          appendNewEntry(&currentDateEntry);
          previousSeatedTime = 0;
          totalSeatedTime = 0;
          display.showNumberDecEx(timeToDisplay(totalSeatedTime), 0b11100000, false, 4, 0);
      } 
      if (previousRead == LOW) {
          current = mktime(&timeinfo);
          totalSeatedTime = totalSeatedTime + (current - previousSeatedTime);
          previousSeatedTime = current;
          display.showNumberDecEx(timeToDisplay(totalSeatedTime), 0b11100000, false, 4, 0);
          currentDateEntry.seatedTime = totalSeatedTime;
          updateEntry(&currentDateEntry);
      }
    }
    vTaskDelay(displayInterval / portTICK_PERIOD_MS);
  }
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

int timeToDisplay(time_t x) {
  int hours = x / (60 * 60);
  int minutes = (x / 60) - (hours * 60);
  int seconds = x % 60;

  if (hours) {
    return (hours * 100) + minutes;
  } else {
    return (minutes * 100) + seconds; 
  }
}

Date getCurrentDate() {
  struct tm timeinfo;
  Date today;
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  } else {
    today.year = (uint16_t)(timeinfo.tm_year + 1900);
    today.month = (uint8_t)(timeinfo.tm_mon + 1);
    today.day = (uint8_t)(timeinfo.tm_mday);
  }
  return today;
}

