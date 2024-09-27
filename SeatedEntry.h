#ifndef SEATED_ENTRY_H
#define SEATED_ENTRY_H

#include <stdbool.h>
#include <time.h>
#include <Arduino.h> 

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
} Date;

typedef struct {
    Date date;
    time_t seatedTime;
} seatedEntry;

bool compareToCurrentDay(seatedEntry* xEntry);
void printSeatedEntry(seatedEntry* xEntry);
void printAllEntries();
void createNewEntry(seatedEntry* xEntry);
void appendNewEntry(seatedEntry* xEntry);
void appendEntry(seatedEntry* xEntry);

bool findEntryByDate(seatedEntry* xEntry, Date* date);
bool appendEntryToFile(seatedEntry* xEntry, String fileDir);
bool updateEntry(seatedEntry* xEntry);

void listFiles(const char *dir);
void displaySPIFFSSpaceInfo();

bool operator==(const Date& d1, const Date& d2);
bool operator!=(const Date& d1, const Date& d2);

#endif
