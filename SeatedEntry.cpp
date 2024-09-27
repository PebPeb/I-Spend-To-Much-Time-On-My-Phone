#include "SeatedEntry.h"
#include "time.h"
#include <Arduino.h> 
#include <FS.h>
#include <SPIFFS.h>

String entryMonthFileHeader = "SE"; //seatedEntry

size_t entryPosition = 0;
Date entryPositionDate;
String entryPositionFile;

bool findEntryFileByDate(Date* date, String* fileDir);
bool findEntryByDate(seatedEntry* xEntry, Date* date, String* fileDir);
void writeFile(String path);
bool isValidSEFileName(String fileName, uint16_t* year, uint8_t* month);
bool findEntryByDate(seatedEntry* xEntry, Date* date, String fileDir);

// Compare the given entry to the current day

bool compareToCurrentDay(seatedEntry* xEntry) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  } else {
    if (xEntry->date.day != (int)(timeinfo.tm_mday)) {
      return false;
    }
    if (xEntry->date.month != (int)(timeinfo.tm_mon + 1)) {
      return false;
    }
    if (xEntry->date.year != (int)(timeinfo.tm_year + 1900)) {
      return false;
    }
    return true;
  }
  return false;
}

void printSeatedEntry(seatedEntry* xEntry) {
    Serial.println("");
    Serial.println("-----------------------------------");
    Serial.println("Printing Seated Entry");
    Serial.println("");
    Serial.print("Total Seated Time: ");
    Serial.println(xEntry->seatedTime);
    Serial.print("Day: ");
    Serial.println(xEntry->date.day);
    Serial.print("Month: ");
    Serial.println(xEntry->date.month);
    Serial.print("Year: ");
    Serial.println(xEntry->date.year);
    Serial.println("-----------------------------------");
    Serial.println("");
}

void printAllEntries() {
  Serial.println("--== Printing all Seated Entries ==--");

  File root = SPIFFS.open("/");
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }

  File file = root.openNextFile();
  uint8_t month;
  uint16_t year;
  
  seatedEntry entry;
  size_t entrySize = sizeof(seatedEntry);

  while (file) {
    if (isValidSEFileName(String(file.name()), &year, &month)) {
      Serial.println("File: " + String(file.name()) + ", Size: " + file.size() + ", Number of Entries: " + file.size()/entrySize);
      int i = 0;
      while (file.read((uint8_t*)&entry, entrySize) == entrySize) {
        Serial.print("Entry ");
        Serial.print(i);
        Serial.print(": ");
        Serial.print(entry.date.day);
        Serial.print(" | ");
        Serial.print(entry.date.month);
        Serial.print(" | ");
        Serial.print(entry.date.year);
        Serial.print(" | ");
        Serial.print(entry.seatedTime);
        Serial.println("");
        i = i + 1;
      }
    }

    file = root.openNextFile();
  }

}

bool isValidSEFileName(String fileName, uint16_t* year, uint8_t* month) {
    // Ensure the filename is exactly 8 characters long
    if (fileName.length() != 8) {
        return false;
    }

    // Ensure the filename starts with "SE"
    if (fileName.substring(0, 2) != "SE") {
        return false;
    }

    // Extract the year (4 digits) and ensure it's numeric
    String yearPart = fileName.substring(2, 6);
    for (int i = 0; i < 4; i++) {
        if (!isDigit(yearPart[i])) {
            return false;
        }
    }

    // Extract the month (2 digits) and ensure it's numeric
    String monthPart = fileName.substring(6, 8);
    for (int i = 0; i < 2; i++) {
        if (!isDigit(monthPart[i])) {
            return false;
        }
    }

    // Convert month to an integer and ensure it's between 01 and 12
    int monthInt = monthPart.toInt();
    if (monthInt < 1 || monthInt > 12) {
        return false;
    }

    *year = (uint16_t)(yearPart.toInt());
    *month = (uint8_t)(monthPart.toInt());
    return true;
}

void createNewEntry(seatedEntry* xEntry) {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
    } else {
      xEntry->date.year = timeinfo.tm_year + 1900;
      xEntry->date.month = timeinfo.tm_mon + 1;
      xEntry->date.day = timeinfo.tm_mday;
      xEntry->seatedTime = 0;

      printSeatedEntry(xEntry);
    }
}

void appendEntry(seatedEntry* xEntry) {
    String fileDir;
    if (!findEntryFileByDate(&(xEntry->date), &fileDir)) {
      writeFile(fileDir);
      Serial.println("Added New file");
    }

    appendEntryToFile(xEntry, fileDir);
}

void appendNewEntry(seatedEntry* xEntry) {
    createNewEntry(xEntry);
    String fileDir;
    if (!findEntryFileByDate(&(xEntry->date), &fileDir)) {
      writeFile(fileDir);
#ifdef DEBUG
      Serial.println("Added New file");
#endif
    }

    appendEntryToFile(xEntry, fileDir);
}

// *************************************************************************************
// Working with SPIFFS
// *************************************************************************************

void writeFile(String path) {
  Serial.print("Creating file: ");
  Serial.println(path);

  File file = SPIFFS.open(path, "w");
  if (file) {
    file.close();
  } else {
  }
}

void listFiles(const char *dir) {
  Serial.print("Listing files in directory: ");
  Serial.println(String(dir));

  File root = SPIFFS.open(dir);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    Serial.println("File: " + String(file.name()) + ", Size: " + file.size());
    file = root.openNextFile();
  }
}



void displaySPIFFSSpaceInfo() {
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  size_t freeBytes = totalBytes - usedBytes;

  Serial.print("Total SPIFFS space: ");
  Serial.print(totalBytes);
  Serial.println(" bytes");

  Serial.print("Used SPIFFS space: ");
  Serial.print(usedBytes);
  Serial.println(" bytes");

  Serial.print("Free SPIFFS space: ");
  Serial.print(freeBytes);
  Serial.println(" bytes");
}

// *************************************************************************************
// 
// *************************************************************************************


bool findEntryByDate(seatedEntry* xEntry, Date* date) {
  String entryFileDir;
  if (!findEntryFileByDate(date, &entryFileDir)) {
    return false;
  }

  if (!findEntryByDate(xEntry, date, entryFileDir)) {
    return false;
  }

  return true;
}

bool findEntryFileByDate(Date* date, String* fileDir) {
  String fileName = "/" + entryMonthFileHeader + String(date->year) + (date->month < 10 ? "0" : "") + String(date->month);
  Serial.println("Looking for file Name: " + fileName);

  // Try to open file
  *fileDir = fileName;
  File file = SPIFFS.open(fileName, "r"); 
  if (!file) {
    return false;
  }
  file.close();
  return true;
}

bool findEntryByDate(seatedEntry* xEntry, Date* date, String fileDir) {
    // Open the file in read mode (binary)
    File file = SPIFFS.open(fileDir, "rb"); // "rb" for read binary mode
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }

    // Temporary storage for reading each entry
    seatedEntry entry;
    size_t entrySize = sizeof(seatedEntry);

    // Loop through the file to find the matching entry
    entryPositionDate = *date;
    entryPositionFile = fileDir;
    while (file.read((uint8_t*)&entry, entrySize) == entrySize) {
        // Compare the date fields
        if (entry.date.year == date->year &&
            entry.date.month == date->month &&
            entry.date.day == date->day) {
            // If a match is found, copy the entry to foundEntry
            *xEntry = entry;

            // Close the file and return true to indicate a match was found
            file.close();
            Serial.println("Entry found!");
            return true;
        }
      entryPosition += entrySize;
    }

    // No match found, close the file and return false
    file.close();
    Serial.println("Entry not found.");
    return false;
}

bool appendEntryToFile(seatedEntry* xEntry, String fileDir) {
    // Open the file in append mode (binary)
    File file = SPIFFS.open(fileDir, "ab"); // "ab" for append binary mode
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    // Write the entry to the file as binary
    size_t written = file.write((uint8_t*)xEntry, sizeof(seatedEntry));
    if (written != sizeof(seatedEntry)) {
      Serial.println("Failed to write entry to file.");
      return false;
    } 
  
    file.close();
    return true;
}

bool updateEntry(seatedEntry* xEntry) {  
  if (xEntry->date != entryPositionDate) {  // Check if date is the same of current position saved
    seatedEntry nullSeatedEntry = *xEntry;
    if (!findEntryByDate(&nullSeatedEntry, &nullSeatedEntry.date, entryPositionFile)) { // Update postion var without overwriting xEntry
      Serial.println("No matching entry found.");
      return false;
    }
  }

  size_t entrySize = sizeof(seatedEntry);

  // Overwrite
  File file = SPIFFS.open(entryPositionFile, "r+b");
  file.seek(entryPosition, SeekSet);
  size_t written = file.write((uint8_t*)xEntry, entrySize);
  file.close();
  

  // Check if the write was successful
  if (written == entrySize) {
#ifdef DEBUG
      Serial.println("Entry overwritten successfully.");
#endif  
  } else {
      Serial.println("Failed to overwrite the entry.");
  }
  return written == entrySize;
}


// Overload the == operator for Date
bool operator==(const Date& d1, const Date& d2) {
    return (d1.year == d2.year &&
            d1.month == d2.month &&
            d1.day == d2.day);
}

// Overload the != operator for Date
bool operator!=(const Date& d1, const Date& d2) {
    return !(d1 == d2);
}

