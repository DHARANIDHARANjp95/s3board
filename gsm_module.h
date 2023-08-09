#include <HardwareSerial.h>
#include "FS.h"
#include <LittleFS.h>
#include <Update.h>

typedef enum
{
  ATTRIBUTE,
  TELEMETRY,
  SHAFT_VERSION,
  CABIN_VERSION,
  LIFT_STATUS,
  SHAFT_OTA_INIT,
  SHAFT_OTA_ACK,
} call_type;

typedef enum 
{
    SHAFT_OTA,
    CABIN_OTA,
    DEVICE_OTA
}OTA_device;

void gsm_init();
void getData();
void publishSerialData(String s);


void connectServer(char * _server, int _port);
void makeRequest(char* _server, char* urlPath);
int readRequest();
void printPercent(uint32_t readLength, uint32_t contentLength);
void performUpdate(Stream &updateSource, size_t updateSize);
int GSMcount();
bool pushChange();
bool connectApn();
void otaRoutine();

void writeFirmware(int _num);
bool downloadFirmware(OTA_device dv);


typedef enum
{
  OTA_IDLE,
  OTA_POSSIB,
  OTA_NOT_NEEDED,
  OTA_SEND_ACK_GSM,
  OTA_SEND_DEC_GSM,
  OTA_AWAIT_ACK_GSM,
  OTA_SEND_ACK_CABIN,
  OTA_SEND_DEC_CABIN,
  OTA_COMPLETE_END_CABIN,
  OTA_DISCONNECT_ESP_NOW,
  OTA_CONNECT_WIFI,
  OTA_CONNECT_SERVER,
  OTA_DOWNLOAD_FILE,
  OTA_UPDATE,
  OTA_ERROR,
  OTA_COMPLETE
} ota_stages;