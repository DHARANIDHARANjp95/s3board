#include <HardwareSerial.h>
#include "FS.h"
#include <LittleFS.h>
#include <Update.h>

void gsm_init();
void getData();
void publishSerialData(String s);
void connectAWSServer();
void makeRequest();
int readRequest();
void writeResponse();
void printPercent(uint32_t readLength, uint32_t contentLength);
void performUpdate(Stream &updateSource, size_t updateSize);
void otaTimer(bool state);
int GSMcount();
bool pushChange();
bool connectApn();
void otaRoutine();