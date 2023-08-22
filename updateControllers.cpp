#include "updateControllers.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>

#include "LittleFS.h"
#include "gsm_module.h"
#include "ShaftLog.h"
AsyncWebServer localServer(80);

const char *ssid = "controller_device";
const char *password = "strongpassword";

bool serverStarted;
bool cabinVersionAvailable;
bool updateGoingOn = false;
SERVER_STATE local_state = SERVER_IDLE;

void setHotspot();
void shaftServing();
void cabinServing();
int serveType = -1;

const char* CABIN_PARAM = "message";
const char* SHAFT_PARAM = "message";

SERVER_OTA_STATUS current_ota_status = OTA_INIT;

void webServerState()
{
    static long int webServerTimeout = millis();
    switch(local_state)
    {
        case SERVER_IDLE:
        {
            //do nothing
        }
        break;
        case HOTSPOT_ENABLE:
        {
            current_ota_status = OTA_ONGOING;
            setHotspot();
            if(serveType == 0)
            {
                local_state = SERVER_ENABLE_SHAFT;
            }
            else if(serveType == 1)
            {
                local_state = SERVER_ENABLE_CABIN;
            }
            else
            {
                local_state = SERVER_ENABLE_CABIN;
            }
            serveType = -1;
            webServerTimeout = millis();
        }
        break;
        case SERVER_ENABLE_SHAFT:
        {
            Serial.println("Shaft serving");
            shaftServing();
            local_state = SERVER_SERVE;
        }
        break;
        case SERVER_ENABLE_CABIN:
        {
            Serial.println("cabin serving");
            cabinServing();
            local_state = SERVER_SERVE;
        }
        break;
        case SERVER_SERVE:
        {
            localServer.begin();
            Serial.println("serving do nothing");
            local_state = SERVER_KEEP_ALIVE;
        }
        break;
        case SERVER_KEEP_ALIVE:
        {
            if(millis() - webServerTimeout > 45*1000)
            {
                Serial.println("Timeout");
                webServerTimeout = millis();
                current_ota_status = OTA_TIMEOUT;
                local_state = SERVER_END;
            }
        }
        break;
        case SERVER_END:
        {
            bool state = WiFi.softAPdisconnect(true);
            localServer.end();
            updateGoingOn=false;
            local_state = SERVER_IDLE;
        }
        break;
        default:
        {
            Serial.println("exception case");
            local_state = SERVER_IDLE;
            current_ota_status = OTA_INIT;
        }
        break;    
    }
}

void setHotspot()
{
  Serial.println("Starting async server");
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(ssid, password); 
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);  
  updateGoingOn = true;
}

void setServer(Update_type up_ty)
{
    if(up_ty == SHAFT_BOARD)
    {
        local_state =SERVER_ENABLE_SHAFT;
    }
    else if(up_ty == CABIN_BOARD)
    {
        local_state =SERVER_ENABLE_CABIN;
    }
}

void shaftServing()
{
    localServer.on("/fwUpdate.bin", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("serving download");   
    request->send(LittleFS, "/fwUpdate.bin", "application/octet-stream");
  });
}

void cabinServing()
{
    localServer.on("/swUpdate.bin", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("serving download");   
    request->send(LittleFS, "/swUpdate.bin", "application/octet-stream");
  });

    localServer.on("/cabinGet", HTTP_GET, [] (AsyncWebServerRequest *request) 
    {
        String versionMsg = "{\"version\":"+getOnlineCabinVersion()+"}";
        String message;
        if (request->hasParam(CABIN_PARAM)) 
        {
            message = request->getParam(CABIN_PARAM)->value();
        } else {
            message = "No message sent";
        }
        if(message == "version")
        {
            request->send(200, "text/plain", versionMsg);
        }
        else if(message == "error")
        {
            current_ota_status = OTA_FAIL;
        }
        else if(message = "sucess")
        {
            setCabinSucess();
            current_ota_status = OTA_SUCESS;
        }
    });
}

void enableHotspot(Update_type TY)
{
    setTimer(millis());
    local_state = HOTSPOT_ENABLE;
    if(TY == SHAFT_BOARD)
    {
        serveType = 0;
    }else
    {
        serveType = 1;
    }
}

void stopServer()
{
    local_state = SERVER_END;
}

SERVER_OTA_STATUS getOTAStatus()
{
    return current_ota_status;
}

bool updateInProgress()
{
    return updateGoingOn;
}