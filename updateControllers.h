#include <HardwareSerial.h>


typedef enum
{
    SERVER_IDLE,
    HOTSPOT_ENABLE,
    SERVER_ENABLE_SHAFT,
    SERVER_ENABLE_CABIN,
    SERVER_SERVE,
    SERVER_KEEP_ALIVE,
    SERVER_END
}SERVER_STATE;

typedef enum{
    OTA_INIT,
    OTA_FAIL,
    OTA_SUCESS,
    OTA_ONGOING,
    OTA_TIMEOUT
}SERVER_OTA_STATUS;

typedef enum{
    SHAFT_BOARD,
    CABIN_BOARD,
}Update_type;

void enableHotspot(Update_type TY);
void setServer(Update_type up_ty);
void stopServer();
void webServerState();

SERVER_OTA_STATUS getOTAStatus();


bool updateInProgress();