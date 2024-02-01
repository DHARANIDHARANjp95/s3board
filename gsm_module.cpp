#include "FS.h"
#include <LittleFS.h>
#include <Update.h>

#include "indicator.h"
#include "updateControllers.h"
#include "gsm_module.h"
#include "ShaftLog.h"
#include <ArduinoJson.h>
#include "common.h"
#include <bits/stdc++.h>
using namespace std;

struct product {
    int x;
    String y;

    product (int i, String j) : x(i), y(j) {}
} ;

queue<product> gsmMessage;

void publishSerialData(product s);
product getFirstMsg();


#define TINY_GSM_MODEM_SIM7600
#define SerialMon Serial  

#define TINY_GSM_RX_BUFFER 650

struct{
    String cabin_version = "";
    String shaft_version = "";
    String shaft_online = "";
    String cabin_online = "";
    String authKey = "";
    String shaftURL = "";
    String cabinURL = "";
    bool shaftFirmwareDownloaded = false;
    bool cabinFirmwareDownloaded = false;  
}version_details;


#define TINY_GSM_USE_GPRS true
#define CHUNK_SIZE (2*1024)

// Your GPRS credentials, if any
const char apn[]      = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char user[] = "";
const char pass[] = "";
// Server details
char* IoTserver  = "13.126.126.206";
const int  IoTport       = 8080;

char* awsserver = "esp32otabalajitest.s3.ap-south-1.amazonaws.com";
const int awsport = 80;

long int cabinTimer = millis();
long int shaftTimer = millis();
long int controllerTImer = millis();
long int reAlertTimer = millis();

int timedOut = 0;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsm        modem(Serial2);

TinyGsmClient client(modem);
HttpClient    http(client, IoTserver, IoTport);

String message="";

String makerId = "ASM_S3_PRO_1";
String controllerType="ltemodules3";

float cur_version = 1.2;
float next_version = cur_version+0.1;

char urlResource[80];
char shaftResource[80];
char iotResource[100];

void deletePreviosuFiles();

#define BLINK_LED 2

typedef enum
{
  ATTRIBUTE,
  TELEMETRY,
  SHAFT_VERSION,
  SHAFT_KEY,
  SHAFT_NEW_VERSION,
  CABIN_VERSION,
  LIFT_STATUS,
  SHAFT_OTA_INIT,
  SHAFT_OTA_ACK,
  SHAFT_OTA_COMPLETE,
  CABIN_OTA_COMPLETE,
} call_type;

int pushedMsgCount = 0;


bool prevBusyState;

uint32_t contentLength = 0;
long timeout;

int otaCheck = 5;
int otaDelay = otaCheck*60*1000; // 3000s => 5minutes
bool otaStatus =true;
bool otaTimerStart;
int otaDisableTime;

File file;

void initSpiffs();
DynamicJsonDocument writeAttributeResponse();

bool deviceUpdate();
void downloadControllerFirmware();

bool  downloadShaftFirmware();
bool  downloadCabinFirmware();

void makeAttributeRequest(String _buff);
void updateFromFS();
void setAuthKey(String _authkey);

bool  setShaftAttributes(String _fwUrl,String _fwVer);
bool  setcabinAttributes(String _swUrl, String _swVer);

void addToProcess(String val, int value);

void resetShaftAlert();

bool shaftUpdateAvailable = false;
bool cabinUpdateAvailable = false;

ota_stages shaft_ota = OTA_DELAY_TIMER;
ota_stages cabin_ota = OTA_DELAY_TIMER;
ota_stages controller_ota = OTA_DELAY_TIMER;
#define DELAY_MINUTES    01
#define DELAY_HOURS      0
#define DELAY_SECONDS    10

#define TIMER_CHECK ((DELAY_HOURS*60*60 + DELAY_MINUTES*60 + DELAY_SECONDS)*1000)

void gsm_init()
{
  
    #ifdef S3_used
      Serial2.begin(115200, SERIAL_8N1, 17, 18);
    #else
      Serial2.begin(115200);
    #endif
    initLED();
    Serial.println("Initializing modem...");
    modem.init();
    String modemInfo = modem.getModemInfo();
    Serial.print("Modem Info: ");
    Serial.println(modemInfo);
    snprintf (urlResource, sizeof(urlResource), "/ASM_S3_PRO_1/%s/%.1f/lte.bin", controllerType,next_version);
    Serial.println(urlResource);
    initSpiffs();
}

void connectInternet()
{
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(100);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }
}

void parseData(String s)
{
  DynamicJsonDocument doc(200);
  deserializeJson(doc, s);
  int msgtype=0;
  int typeVal = doc["type"];
  switch(typeVal)
  {
    case ATTRIBUTE:
    {
        msgtype = 0;
        String _authKey = doc["key"];
        setAuthKey(_authKey);
        String _msg="";
        serializeJson(doc, _msg);
        JsonObject object = doc.as<JsonObject>();
        object.remove("type");
        object.remove("key");
        _msg="";
        serializeJson(doc, _msg);
        Serial.println("message attribute "+_msg);
        gsmMessage.push(product(msgtype, _msg));
    }
    break;
    case TELEMETRY:
    {
        msgtype = 1;
        String _authKey = doc["key"];
        setAuthKey(_authKey);
        String _msg="";
        serializeJson(doc, _msg);
        JsonObject object = doc.as<JsonObject>();
        object.remove("type");
        object.remove("key");
        _msg="";
        serializeJson(doc, _msg);
        Serial.println("message telemetry "+_msg);
        gsmMessage.push(product(msgtype, _msg));
    }
    break;
    case SHAFT_VERSION:
    {
        String _curVersion=doc["state"];
        if(version_details.shaft_version=="")
        {
          String _msg = "{\"shaft_version\":"+_curVersion+"}";
          gsmMessage.push(product(ATTRIBUTE, _msg));
        }
        version_details.shaft_version = _curVersion;
        Serial.println("Shaft version received");
    }
    break;
    case SHAFT_KEY:
    {
        String _authKey = (doc["state"]);
        setAuthKey(_authKey);
        Serial.println("auth key received");
    }
    break;
    case SHAFT_NEW_VERSION:
    {

    }
    break;
    case CABIN_VERSION:
    {
        String _curVersion=doc["state"]; 
        if(version_details.cabin_version=="")
        {
          String _msg = "{\"cabin_version\":"+_curVersion+"}";
          gsmMessage.push(product(ATTRIBUTE, _msg));
        }
        version_details.cabin_version = _curVersion;
        Serial.println("cabin version received"+String(_curVersion));
    }
    break;
    case LIFT_STATUS:
    {

    }
    break;
    case SHAFT_OTA_INIT:
    {   
      if(doc["state"]=="busy")
      {
          Serial.println("shaft busy");
          shaft_ota = OTA_ERROR;   
      }
      else if(doc["state"]=="busy_again")
      {
          Serial.println("shaft busy after downloading");
          resetShaftAlert();
      }
      else if(doc["state"]=="idle")
      {
          Serial.println("shaft idle");
          shaft_ota = OTA_DOWNLOAD_FILE;
      }
      else if(doc["state"]== "ready")
      {
          Serial.println("update ready");
          shaft_ota = OTA_CONNECT_WIFI;
      }
      else if(doc["state"]== "same")
      {
          Serial.println("update not needed");
          shaft_ota = OTA_IDLE;
      }
      else if(doc["state"]=="true")
      {
          Serial.println("update done");
          String _msg = "{\"shaft_update\":\"complete\"}";
          gsmMessage.push(product(ATTRIBUTE, _msg));
          shaft_ota = OTA_COMPLETE;
          setShaftSucess();
      }   
    }
    break;
    case SHAFT_OTA_ACK:
    {

    }
    break;
    case SHAFT_OTA_COMPLETE:
    {

    }
    break;
    case CABIN_OTA_COMPLETE:
    {

    }
    break;
    default:
    {

    }
    break;
  }
  doc.clear();
}


void initSpiffs()
{
  if (!LittleFS.begin(true))
  {
      Serial.println("LittleFS Mount Failed");
      LittleFS.format();
  }
}

bool connectServer(char * _server, int _port)
{
  Serial.print("Connecting to ");
  Serial.print(_server);
  bool state = false;
  // if you get a connection, report back via serial:
  if (!client.connect(_server, _port))
  {
      Serial.println(" fail");
  }
  else
  {
    state = true;
    Serial.println("Connected to server");
  }
  return state;
}



void makeRequest(char* _server, char* urlPath)
{
  // Make a HTTP request:

  client.print(String("GET ") + String(urlPath) + " HTTP/1.0\r\n");
  client.print(String("Host: ") + _server + "\r\n");
  client.print("Connection: close\r\n\r\n");

  timeout = millis();
  while (client.available() == 0)
  {
      if (millis() - timeout > 1000L)
      {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return;
      }
  }
}

int readRequest()
{
  Serial.println("Reading header");
  contentLength=0;
    while (client.available())
    {
        String line = client.readStringUntil('\n');
        line.trim();    
        //Serial.println(line);    // Uncomment this to show response header
        line.toLowerCase();
        if (line.startsWith("content-length:") || line.startsWith("Content-Length:"))
        {
            contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
        }
        else if (line.length() == 0)
        {
            break;
        }
    }
    Serial.println("Content length = "+ String(contentLength));
    return contentLength;
}

void printPercent(uint32_t readLength, uint32_t contentLength)
{
    // If we know the total length
    if (contentLength != -1)
    {
        Serial.print("\r ");
        Serial.print((100.0 * readLength) / contentLength);
        Serial.print('%');
    }
    else
    {
        Serial.println(readLength);
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void updateFromFS()
{
    File updateBin = LittleFS.open("/update.bin");
    if (updateBin)
    {
        if (updateBin.isDirectory())
        {
            Serial.println("Directory error");
            updateBin.close();
            return;
        }

        size_t updateSize = updateBin.size();

        if (updateSize > 0)
        {
            Serial.println("Starting update");
            performUpdate(updateBin, updateSize);
        }
        else
        {
            Serial.println("File not found");
        }

        updateBin.close();

        // whe finished remove the binary from sd card to indicate end of the process
        LittleFS.remove("/update.bin");
    }
    else
    {
        Serial.println("binary file not available");
        return;
    }
}


void performUpdate(Stream &updateSource, size_t updateSize)
{
  //REQ_STATUS espRequest;
    if (Update.begin(updateSize))
    {
        size_t written = Update.writeStream(updateSource);
        if (written == updateSize)
        {
            Serial.println("Writes : " + String(written) + " successfully");
        }
        else
        {
            Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
        }
        if (Update.end())
        {
            Serial.println("OTA finished!");
            if (Update.isFinished())
            {
                //espRequest = espCabinRequest(&sendUpdateNotification,(uint32_t)false);
                Serial.println("Restart ESP device!");
                ESP.restart();
            }
            else
            {
                Serial.println("OTA not fiished");
            }
        }
        else
        {
            Serial.println("Error occured #: " + String(Update.getError()));
        }
    }
    else
    {
        Serial.println("Cannot begin update");
    }
}

void otaRoutine()
{
  downloadControllerFirmware();
}

bool deviceUpdate()
{
  static bool deviceUpdateProcessComplete = false;
  switch(controller_ota)
  {
  case OTA_IDLE:
  { 
    Serial.println("Controller will check and download new verison");
    controller_ota = OTA_DOWNLOAD_FILE;
  }
  break;
  case OTA_DOWNLOAD_FILE:
  {
    bool state = downloadFirmware(DEVICE_OTA);
    if(state)
    {
      Serial.println("Controller will check for pending logs now");
      controller_ota = OTA_UPDATE;
    }
    else
    {
      Serial.println("Controller will check shaft OTA now");
      controller_ota = OTA_ERROR;
    }
  }
  break;
  case OTA_CHECK_AGAIN:
  {
    if(emptyQueue())
    {
      Serial.println("No logs are pending - Controller can restart now");
      controller_ota = OTA_UPDATE;
    }
  }
  break;
  case OTA_DELAY_TIMER:
  {
    deviceUpdateProcessComplete = false;
    if(millis() - controllerTImer > (TIMER_CHECK))
    {
      Serial.println("Timer over - Controller can check OTA now");
      controllerTImer=millis();
      controller_ota = OTA_IDLE;
    }
  }
  break;
  case OTA_UPDATE:
  {
    updateFromFS();
    controller_ota = OTA_COMPLETE;
  }
  break;
  case OTA_ERROR:
  {
    controller_ota = OTA_DELAY_TIMER;
    controllerTImer=millis();
    deviceUpdateProcessComplete=true;
  }
  break;
  case OTA_COMPLETE:
  {
    deviceUpdateProcessComplete = false;
    ESP.restart();
  }
  break;
  default:
  {

  }
  break;
  }
  return deviceUpdateProcessComplete;
}

bool downloadFirmware(OTA_device dv)
{
  bool status = false;
  char endPoint[80];
  switch (dv)
  {
  case SHAFT_OTA:
  {
    version_details.shaftURL.toCharArray(endPoint, version_details.shaftURL.length() + 1);
  }
  break;
  case CABIN_OTA:
  {
    version_details.cabinURL.toCharArray(endPoint, version_details.cabinURL.length() + 1);
  }
  break;
  case DEVICE_OTA:
  {
    memcpy(endPoint, urlResource, sizeof(urlResource));
  }
  break;
  default:
    break;
  }
  Serial.print("endPoint = ");
  for(int i=0;i<(version_details.shaftURL.length()+1);i++)
  {
    Serial.print(endPoint[i]);
  }
  Serial.println();

  if(connectApn())
  {
    if(connectServer(awsserver, awsport))
    {
      makeRequest(awsserver, endPoint);
      if(readRequest())
      {
        writeFirmware(dv); 
        status = true;
      } 
      else
      {
        Serial.println("empty content");
      } 
    }
  }
  return status;
}

bool pushChange()
{
    bool state = false;
    static long int prevValue = 0;
    if(prevValue!=pushedMsgCount)
    {
        prevValue=pushedMsgCount;
        state = true;
    }
    return state;
}

bool connectApn()
{
//   Serial.print("Connecting to ");
//   Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass))
  {
    Serial.println("apn fail");
    return false;
  }
  else
  {
    return true;
  }
}

DynamicJsonDocument writeAttributeResponse()
{
    timeout = millis();
    uint32_t readLength = 0;
    Serial.println("Content size - "+ String(contentLength));
    //#define max_size 1*1024
    String attribResponse;   
    while (readLength < contentLength && client.connected() && millis() - timeout < 10000L)
    {
        attribResponse+=char(client.read());
    }
    Serial.println(attribResponse);
    DynamicJsonDocument doc(contentLength+50);
    deserializeJson(doc, attribResponse);
    return doc;
}


void writeFirmware(int _num)
{
    timeout = millis();
    uint32_t readLength = 0;

    unsigned long timeElapsed = millis();
    deletePreviosuFiles();
    //printPercent(readLength, contentLength);
    if(_num == SHAFT_OTA)
    {
      file = LittleFS.open("/fwUpdate.bin", FILE_APPEND);
      Serial.println("Delete and append shaft file");
    } 
    else if(_num == CABIN_OTA)
    {
      file = LittleFS.open("/swUpdate.bin", FILE_APPEND);
      Serial.println("Delete and append cabin file");
    }
    else if(_num == DEVICE_OTA)
    {
      file = LittleFS.open("/update.bin", FILE_APPEND);     
      Serial.println("Delete and append device file"); 
    }

	int downloadRemaining = contentLength;
	
	uint8_t* buffer_ = (uint8_t*)malloc(CHUNK_SIZE);
	uint8_t* cur_buffer = buffer_;
	while ( downloadRemaining > 0 && client.connected() ) 
	{
		while (client.available()) 
		{
		  
		  auto available_buffer_size = CHUNK_SIZE - (cur_buffer - buffer_);
		  
		  auto read_count = client.read(cur_buffer, ((downloadRemaining > available_buffer_size) ? available_buffer_size : downloadRemaining));
		  cur_buffer += read_count;
		  downloadRemaining -= read_count;
		  // If one chunk of data has been accumulated, write to SD card
		  if ((cur_buffer - buffer_ == read_count) &&(read_count))
      {
        Serial.println("read count = "+String(read_count));
        file.write(buffer_, read_count);
			  cur_buffer = buffer_;
		  }
		}
	}
  file.close();  
  Serial.println("write firmware process over");
}

void setAuthKey(String _authkey)
{
  if(_authkey!="")
  {
    Serial.println("received auth key "+ _authkey);
    version_details.authKey = _authkey;
  }
}


void makeAttributeRequest(String _buff)
{
  // Make a HTTP request:
  client.print(String("GET ") + _buff + " HTTP/1.0\r\n"); //
  client.print(String("Host: ") + IoTserver + "\r\n");
  //client.print(String("Cache-Control: max-age=0 ")+ "\r\n"); //no-cache, no-store, max-age=0, must-revalidate
  client.print(String("Cache-Control: no-cache, no-store, max-age=0, must-revalidate ")+ "\r\n"); //no-cache, no-store, max-age=0, must-revalidate
  client.print("Connection: close\r\n\r\n");

  timeout = millis();
  while (client.available() == 0)
  {
      if (millis() - timeout > 5000L)
      {
          Serial.println(">>> Client Timeout !");
          client.stop();
          delay(1000L);
          return;
      }
  }
}

bool  setShaftAttributes(String _fwUrl,String _fwVer)
{
  version_details.shaftURL = _fwUrl;
  version_details.shaft_online = _fwVer;
  if((version_details.shaft_version!=""))
  {
    if(version_details.shaft_version != version_details.shaft_online)
    {
      Serial.println("Shaft Version changed");
      Serial.println("shaft version is "+version_details.shaft_version+ " new version is "+_fwVer);
      if(version_details.shaftURL!=NULL)
      {
        shaftUpdateAvailable = true;
        addToProcess(_fwVer, SHAFT_NEW_VERSION);
      }
      else
      {
        Serial.println("Shaft version null value");
        shaftUpdateAvailable = false; 
      }
      //send message to shaft that update is available
    }
    else
    {
      Serial.println("Shaft version same");
      shaftUpdateAvailable=false;
    }
  }
  else
  {
    shaftUpdateAvailable=false;
    Serial.println("Shaft version unavailable");
  }
  
  return shaftUpdateAvailable;
}

bool setcabinAttributes(String _swUrl, String _swVer)
{
  bool state = false;
  version_details.cabinURL = _swUrl;
  version_details.cabin_online = _swVer;
  if(version_details.cabin_version!="")
  {
    if(version_details.cabin_version != version_details.cabin_online)
    {
        Serial.println("cabin update message to shaft");
        state = true;
    }
    else
    {
      Serial.println("cabin same version");
    }
  }
  else
  {
    Serial.println("cabin version unavailable");
  }

  return state;
}

void downloadControllerFirmware()
{
  webServerState();
  static int i =2;
  switch(i)
  {
    case 0:
    {
      bool state = deviceUpdate();
      if(state)
      {
        i++;
      }
    }
    break;
    case 2:
    {
      bool state = downloadShaftFirmware();
      if(state)
      {
        i++;
      }
    }
    break;
    case 1:
    {
      bool state = downloadCabinFirmware();
      if(state)
      {
        i++;
      }
    }
    break;
    default:
    {
      i=0;
    }
    break;
  }
}

bool downloadShaftFirmware()
{
  static bool prevShaftState = false;
  static bool shaftUpdateProcessComplete = false;
  switch(shaft_ota)
  {
    case OTA_IDLE:
    {
      Serial.println("OTA_IDLE");
      if(version_details.authKey!="")
      {
        shaft_ota = OTA_GET_ONLINE_VERSION;
      }
      else
      {
        Serial.println("shaft authentication key unavailable.");
        shaft_ota = OTA_ERROR;
      }
    }
    break;
    case OTA_GET_ONLINE_VERSION:
    {
      char swurl[100];
      snprintf(iotResource, sizeof(iotResource), "/api/v1/%s/attributes?sharedKeys=fw_version,fw_url",version_details.authKey);
      Serial.println(iotResource);
      client.stop();
      if(connectApn())
      {
        Serial.println("shaft");
        if(connectServer(IoTserver,IoTport))
        {
          makeAttributeRequest(String(iotResource));
          if(readRequest()>2)
          {
            DynamicJsonDocument doc(200);
            doc = writeAttributeResponse();
            String shafURL = doc["shared"]["fw_url"];
            String shafVer = doc["shared"]["fw_version"]; 
            Serial.println(" Shaftver: "+shafVer+" version_details.Shafturl: "+shafURL);
            bool status = setShaftAttributes(shafURL, shafVer);
            if(status)
            {
              shaft_ota=OTA_DOWNLOAD_FILE;
            }
            else
            {
              shaft_ota = OTA_ERROR;
            }
          }
          else
          {
            shaft_ota = OTA_ERROR;
          }
        }
        else
        {
          shaft_ota = OTA_ERROR;
        }
      }
      else
      {
        shaft_ota = OTA_ERROR;
      }      
    }
    break;
    case OTA_DELAY_TIMER:
    {
      shaftUpdateProcessComplete=false;
      if(millis() - shaftTimer > (TIMER_CHECK))
      {
        shaftTimer=millis();
        shaft_ota = OTA_IDLE;
      }
    }
    break;
    case OTA_AWAIT_ACK_GSM:
    {
      if(millis() - reAlertTimer > 40000)
      {
        Serial.println("update retry");
        reAlertTimer = millis();
        addToProcess("true", SHAFT_OTA_ACK);
        timedOut++;
      }
      if(timedOut > 3)
      {
        Serial.println("ignoring this update. will check cabin");
        shaft_ota = OTA_ERROR;
      }
    }
    
    break;
    case OTA_DOWNLOAD_FILE:
    {
      bool state = downloadFirmware(SHAFT_OTA);
      if(state)
      {
        //send acknowlegement
        version_details.shaftFirmwareDownloaded = true;
        addToProcess("true", SHAFT_OTA_ACK);
        Serial.println("Download complete - start wifi");
        shaft_ota = OTA_CHECK_AGAIN;
      }
      else
      {
        shaft_ota = OTA_ERROR;
      }

    }
    break;
    case OTA_CHECK_AGAIN:
    {
      //do nothing
    }
    break;
    case OTA_CONNECT_WIFI:
    {
      enableHotspot(SHAFT_BOARD);
      shaft_ota = OTA_CONNECT_SERVER;
    }
    break;
    case OTA_CONNECT_SERVER:
    {
      //setServer(SHAFT_BOARD);
      Serial.println("OTA_CONNECT_SERVER");
      shaft_ota = OTA_UPDATE;
    }
    break;
    case OTA_UPDATE:
    {
      SERVER_OTA_STATUS status = getOTAStatus();
      switch(status)
      {
        case OTA_INIT:
        {

        }
        break;
        case OTA_FAIL:
        {
          Serial.println("fail setting sequence to next error step");
          shaft_ota = OTA_ERROR;
        }
        break;
        case OTA_SUCESS:
        {
          Serial.println("sucess setting sequence to next step");
          shaft_ota = OTA_COMPLETE;
          setShaftSucess();
        }
        break;
        case OTA_ONGOING:
        {

        }
        break;
        case OTA_TIMEOUT:
        {
          Serial.println("timeout setting sequence to idle");
          shaft_ota = OTA_ERROR;
        }
        break;
        default:
        {
          Serial.println("invalid ota state reached");
          shaft_ota = OTA_ERROR;
        }
        break;
      }
    }
    break;
    case OTA_ERROR:
    {
      Serial.println("OTA_ERROR");
      stopServer();
      setShaftDelay();
      shaftUpdateProcessComplete = true;
    }
    break;
    case OTA_COMPLETE:
    {
      Serial.println("OTA_COMPLETE");
      stopServer();
      setShaftDelay();
      shaftUpdateProcessComplete = true;
    }
    break;
    default:
    {
      setShaftDelay();
      Serial.println("caught error");
    }
    break;
  }
  return shaftUpdateProcessComplete;
}

bool downloadCabinFirmware()
{
  static bool prevShaftState = false;
  static bool cabinUpdateProcessComplete = false;
  switch(cabin_ota)
  {
    case OTA_IDLE:
    {
      if(version_details.authKey!="")
      {
        cabin_ota = OTA_GET_ONLINE_VERSION;
      }
      else
      {
        Serial.println("cabin authentication key unavailable.");
        cabin_ota = OTA_ERROR;
      }
    }
    break;
    case OTA_GET_ONLINE_VERSION:
    {
      char swurl[100];
      snprintf (iotResource, sizeof(iotResource), "/api/v1/%s/attributes?sharedKeys=sw_version,sw_url",version_details.authKey);
      Serial.println(iotResource);
      client.stop();
      if(connectApn())
      {
        if(connectServer(IoTserver,IoTport))
        {
          makeAttributeRequest(String(iotResource));
          int content_len = readRequest();
          if(content_len>2)
          {
            DynamicJsonDocument doc(200);
            doc = writeAttributeResponse();
            Serial.println("cabin has key");
            String cabURL = doc["shared"]["sw_url"];
            String cabVER = doc["shared"]["sw_version"]; 
            Serial.println(" Cabin Version: "+cabVER+" Cabin url: "+cabURL);
            bool state = setcabinAttributes(cabURL, cabVER);
            if(state)
            {
                cabin_ota=OTA_POSSIB;
            }
            else
            {
              Serial.println("download not available yet");
              cabin_ota = OTA_ERROR;
            }
          }
          else
          {
            Serial.println("received content length = "+String(content_len));
            cabin_ota = OTA_ERROR;
          }
        }
        else
        {
          Serial.println("connect server failed");
          cabin_ota = OTA_ERROR;
        }
      }
      else
      {
        Serial.println("connect internet failed");
        cabin_ota = OTA_ERROR;
      }      
    }
    break;
    case OTA_DELAY_TIMER:
    {
      cabinUpdateProcessComplete=false;
      if(millis() - cabinTimer > (TIMER_CHECK))
      {
        cabinTimer=millis();
        cabin_ota = OTA_IDLE;
      }
    }
    break;
    case OTA_POSSIB:
    {
      Serial.println("cabin consider possibility");
      cabin_ota = OTA_DOWNLOAD_FILE;
    }
    break;
    case OTA_AWAIT_ACK_GSM:
    {
      static long int timer = millis();
      if(millis() - timer > 40000)
      {
        timer=millis();
      }
    }
    break;
    case OTA_DOWNLOAD_FILE:
    {
      Serial.println("cabin download");
      bool state = downloadFirmware(CABIN_OTA);
      if(state)
      {
        //send acknowlegement
        version_details.cabinFirmwareDownloaded = true;
        addToProcess("true", CABIN_VERSION);
        Serial.println("Download complete - start wifi");
        cabin_ota = OTA_CONNECT_WIFI;
      }
      else
      {
        cabin_ota = OTA_ERROR;
      }

    }
    break;
    case OTA_CONNECT_WIFI:
    {
      enableHotspot(CABIN_BOARD);
      cabin_ota = OTA_CONNECT_SERVER;
    }
    break;
    case OTA_CONNECT_SERVER:
    {
      //setServer(SHAFT_BOARD);
      cabin_ota = OTA_UPDATE;
    }
    break;
    case OTA_UPDATE:
    {
      SERVER_OTA_STATUS status = getOTAStatus();
      switch(status)
      {
        case OTA_INIT:
        {

        }
        break;
        case OTA_FAIL:
        {
          Serial.println("fail setting sequence to next error step");
          cabin_ota = OTA_ERROR;
        }
        break;
        case OTA_SUCESS:
        {
          Serial.println("sucess setting sequence to next step");
          cabin_ota = OTA_COMPLETE;
        }
        break;
        case OTA_ONGOING:
        {

        }
        break;
        case OTA_TIMEOUT:
        {
          Serial.println("timeout setting sequence to idle");
          cabin_ota = OTA_ERROR;
        }
        break;
        default:
        {
          Serial.println("invalid ota state reached");
          cabin_ota = OTA_ERROR;
        }
        break;
      }
    }
    break;
    case OTA_ERROR:
    {
      stopServer();
      cabin_ota = OTA_IDLE;
      setCabinDelay();
      cabinUpdateProcessComplete = true;
    }
    break;
    case OTA_COMPLETE:
    {
      stopServer();
      cabin_ota = OTA_IDLE;
      setCabinDelay();
      String _msg = "{\"cabin_update\":\"complete\"}";
      gsmMessage.push(product(ATTRIBUTE, _msg));
      cabinUpdateProcessComplete = true;
    }
    break;
    default:
    {
      Serial.println("caught error");
      setCabinDelay();
    }
    break;
  }
  return cabinUpdateProcessComplete;
}

void addToProcess(String val, int value)
{
  static String prevVal = "";
  if(prevVal != val)
  {
    //prevVal=val;
    Serial1.flush();
    DynamicJsonDocument doc(150);
    doc["type"]=value;
    doc["state"]=val;
    serializeJson(doc,Serial1);  
    Serial.println("&&&&");
    doc.clear(); 
  }
}

void setCabinDelay()
{
  Serial.println("set cabin delay");
  cabinTimer=millis();
  cabin_ota = OTA_DELAY_TIMER;
}


void setShaftDelay()
{
  Serial.println("set shaft delay");
  shaftTimer=millis();
  shaft_ota = OTA_DELAY_TIMER;
}

void setControllerDelay()
{
  Serial.println("set controller delay");
  controllerTImer=millis();
  controller_ota = OTA_DELAY_TIMER;
}

void deletePreviosuFiles()
{
  LittleFS.remove("/fwUpdate.bin");//delete if existing already.
  LittleFS.remove("/swUpdate.bin");//delete if existing already.
  LittleFS.remove("/update.bin");//delete if existing already.
  resetFirmwareDownloadDetails();
}

String getLocalShaftVersion()
{
    return version_details.shaft_version;
}

String getLocalCabinVersion()
{
    return version_details.cabin_version;
}

void setCabinVersion(String version)
{
    version_details.cabin_version = version;
}
void setshaftVersion(String version)
{
    version_details.shaft_version = version;
}

String getOnlineCabinVersion()
{
  return version_details.cabin_online;
}

void setCabinSucess()
{
  version_details.cabin_version = version_details.cabin_online;
}

void setShaftSucess()
{
  version_details.shaft_version = version_details.shaft_online;
}

bool emptyDetails()
{
  bool status = false;
  if((version_details.shaft_online=="") && (version_details.authKey==""))
  {
    status = true;
  }
  return status;
}

void resetShaftAlert()
{
  reAlertTimer = millis();
  shaft_ota = OTA_AWAIT_ACK_GSM;
  timedOut = 0;
}

void resetFirmwareDownloadDetails()
{
  version_details.shaftFirmwareDownloaded = false;
  version_details.cabinFirmwareDownloaded = false;
}

float getVersion()
{
  return cur_version;
}

product getFirstMsg()
{
    return gsmMessage.front();
}

void sendQueuedDataToGSM()
{
  static long int checkInterval = millis();
  if(!emptyQueue() && (!updateInProgress()))
  //if(!emptyQueue())
  {
    Serial.printf("Shaft msg count");
    product newMsg = getFirstMsg();
    publishSerialData(newMsg);
  }
}

void publishSerialData(product s)
{
  int typeVal = s.x;
  String _msg = s.y;
  String msgType = typeVal?"telemetry":"attributes";
  // if(typeVal == ATTRIBUTE)
  // {
  //   msgType = "attributes";
  // }
  // else if(typeVal == TELEMETRY)
  // {
  //   msgType == "telemetry";
  // }
  if((version_details.authKey!=""))
  {
    pushedMsgCount++;
    setIndicator(BLINK_TWICE);
    String coords = "/api/v1/"+version_details.authKey+"/"+msgType;
    Serial.println(coords);
    int err = http.post(coords, "application/json", _msg);
    if (err != 0)
    {
      SerialMon.println(F("failed to connect"));
      delay(100);
      return;
    }
    else
    {
      Serial.println("post connected");
    }
    int status = http.responseStatusCode();
    Serial.printf("status code= %d\n",status);
    String body = http.responseBody();
    SerialMon.println(F("Response:"));
    SerialMon.println(body);
    http.stop();
    SerialMon.println(F("Server disconnected"));
    setIndicator(TURN_OFF);
  }
  else
  {
    Serial.println("publish authentication key unavailable");
  }
}

bool emptyQueue()
{
    return gsmMessage.empty();
}

void popFront()
{
    gsmMessage.pop();
    Serial.println("Popped the first one");
}
