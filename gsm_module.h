#include <HardwareSerial.h>

typedef enum 
{
    SHAFT_OTA,
    CABIN_OTA,
    DEVICE_OTA
}OTA_device;

void gsm_init();
void getData();

bool connectServer(char * _server, int _port);
void makeRequest(char* _server, char* urlPath);
int readRequest();
void printPercent(uint32_t readLength, uint32_t contentLength);
void performUpdate(Stream &updateSource, size_t updateSize);
bool pushChange();
bool connectApn();
void otaRoutine();

void setCabinDelay();
void setShaftDelay();
void setControllerDelay();

void writeFirmware(int _num);
bool downloadFirmware(OTA_device dv);
void sendQueuedDataToGSM();

typedef enum
{
  OTA_IDLE,
  OTA_GET_ONLINE_VERSION,
  OTA_POSSIB,
  OTA_DELAY_TIMER,
  OTA_NOT_NEEDED,
  OTA_SEND_ACK_GSM,
  OTA_SEND_DEC_GSM,
  OTA_AWAIT_ACK_GSM,
  OTA_SEND_ACK_CABIN,
  OTA_SEND_DEC_CABIN,
  OTA_COMPLETE_END_CABIN,
  OTA_DISCONNECT_ESP_NOW,
  OTA_CHECK_AGAIN,
  OTA_CONNECT_WIFI,
  OTA_CONNECT_SERVER,
  OTA_DOWNLOAD_FILE,
  OTA_UPDATE,
  OTA_ERROR,
  OTA_COMPLETE
} ota_stages;

float getVersion();

void setCabinVersion(String version);
void setshaftVersion(String version);

String getLocalShaftVersion();
String getLocalCabinVersion();
String getOnlineCabinVersion();

bool emptyDetails();
void setCabinSucess();
void setShaftSucess();
void parseData(String s);

void resetFirmwareDownloadDetails();

void popFront();