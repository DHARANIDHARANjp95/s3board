#include <HardwareSerial.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>

typedef enum
{
    SERVER_IDLE,
    HOTSPOT_ENABLE,
    SERVER_ENABLE,
    SERVER_START,
    SERVER_SERVE,
    SERVER_BEGIN,
    


}SERVER_STATE;