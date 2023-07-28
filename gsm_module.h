#include <HardwareSerial.h>
#include "FS.h"
#include <LittleFS.h>
#include <Update.h>

void gsm_init();
void getData();
void publishSerialData(String s);
bool connectApn();
void connectAWSServer();
void makeRequest();
void readRequest();
void writeResponse();
void printPercent(uint32_t readLength, uint32_t contentLength);
void performUpdate(Stream &updateSource, size_t updateSize);
void otaTimer(bool state);