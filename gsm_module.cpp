#include "gsm_module.h"
#include <ArduinoJson.h>

#include "updateControllers.h"
#define TINY_GSM_MODEM_SIM7600
#define SerialMon Serial  

#define TINY_GSM_RX_BUFFER 650

String shaftVersion = "1.1";
String cabinVersion = "1.1";

String shaftURL = "";
String cabinURL = "";

String authKey = "factoryG3";
String shaftFile = "shaftFirmware";
String cabinFile = "cabinFirmware";

#define TINY_GSM_USE_GPRS true
#define CHUNK_SIZE 1024
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
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsm        modem(Serial2);

TinyGsmClient client(modem);
HttpClient    http(client, IoTserver, IoTport);

String message="";

String makerId = "ASM_S3_PRO_1";
String sNo = "factoryg3";
String controllerType="debug";
char urlResource[80];
char shaftResource[80];
char iotResource[100];

#define BLINK_LED 2

typedef enum
{
  ATTRIBUTE,
  TELEMETRY,
  SHAFT_VERSION,
  SHAFT_NEW_VERSION,
  CABIN_VERSION,
  LIFT_STATUS,
  SHAFT_OTA_INIT,
  SHAFT_OTA_ACK,
} call_type;

int pushedMsgCount = 0;
float cur_version = 1.0;
float next_version = cur_version+0.1;

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

void deviceUpdate();
void controllerBoardAttributes();
void downloadControllerFirmware();

void  downloadShaftFirmware();
void  downloadCabinFirmware();

void makeAttributeRequest(String _buff);
void updateFromFS();
void setAuthKey(String _authkey);

void  setShaftAttributes(String _fwUrl,String _fwVer);
void  setcabinAttributes(String _swUrl, String _swVer);

void addToProcess(String val, int value);

bool shaftUpdateAvailable = false;
bool cabinUpdateAvailable = false;

ota_stages shaft_ota = OTA_IDLE;
ota_stages cabin_ota = OTA_IDLE;

void gsm_init()
{
    Serial2.begin(115200);
    pinMode(BLINK_LED,OUTPUT);
    Serial.println("Initializing modem...");
    modem.init();
    String modemInfo = modem.getModemInfo();
    Serial.print("Modem Info: ");
    Serial.println(modemInfo);
    //connectInternet();
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

void getData()
{
  SerialMon.print(F("Performing HTTP GET request... "));
  int err = 0;
  http.get(shaftResource);
  if (err != 0) 
  {
    SerialMon.println(F("failed to connect"));
    delay(100);
    return;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    delay(100);
    return;
  }

  SerialMon.println(F("Response Headers:"));
  while (http.headerAvailable()) 
  {
    String headerName  = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    SerialMon.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    SerialMon.print(F("Content length is: "));
    SerialMon.println(length);
  }
  if (http.isResponseChunked()) 
  {
    SerialMon.println(F("The response is chunked"));
  }
  if(message!="")
  {
    Serial.println(message);
    publishSerialData(message);
  }
  // String body = http.responseBody();
  // SerialMon.println(F("Response:"));
  // SerialMon.println(body);

  // SerialMon.print(F("Body length is: "));
  // SerialMon.println(body.length());

  // Shutdown

  // http.stop();
  // SerialMon.println(F("Server disconnected"));
  // modem.gprsDisconnect();
  // SerialMon.println(F("GPRS disconnected"));  
}

void publishSerialData(String s)
{
  DynamicJsonDocument doc(524);
  deserializeJson(doc, s);
  DynamicJsonDocument doc1 = doc;
  String _msg="";
  serializeJson(doc1, _msg);
  JsonObject object = doc1.as<JsonObject>();
  object.remove("type");
  object.remove("key");
  _msg="";
  serializeJson(doc1, _msg);
  Serial.println("message retained ="+_msg);
  String _authKey = (doc["key"]);
  setAuthKey(_authKey);
  String msgtype="";
  int typeVal = doc["type"];
  if(typeVal == ATTRIBUTE)
  {
    msgtype="attributes";
  }
  else if(typeVal == TELEMETRY)
  {
    msgtype="telemetry";
  }
  else if(typeVal == SHAFT_VERSION)
  {
    msgtype = "shaft_version";
  }else if(typeVal == CABIN_VERSION)
  {
    msgtype = "cabin_version";
  }else if(typeVal == SHAFT_OTA_INIT)
  {
    msgtype = "shaft_ota_init";
  }
  //msgtype="telemetry";
  if((typeVal == ATTRIBUTE) || (typeVal == TELEMETRY)  )
  {
      digitalWrite(BLINK_LED,HIGH);
      String coords = "/api/v1/"+authKey+"/"+msgtype;
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
      // int status = http.responseStatusCode();
      // Serial.printf("status code= %d\n",status);
      // String body = http.responseBody();
      // SerialMon.println(F("Response:"));
      // SerialMon.println(body);
      http.stop();
      SerialMon.println(F("Server disconnected"));
      pushedMsgCount++;
      digitalWrite(BLINK_LED,LOW);
  }
  else
  {
    if(typeVal == SHAFT_OTA_INIT)
    {
      if(doc["state"]=="busy")
      {
        //shiftUpdateAvailable = true;
      }
      else if(doc["state"]=="idle")
      {
        addToProcess("true", SHAFT_OTA_ACK);
      }
    }
  }
}

void initSpiffs()
{
  if (!LittleFS.begin(true))
  {
      Serial.println("LittleFS Mount Failed");
      LittleFS.format();
  }
}

void connectServer(char * _server, int _port)
{
  Serial.print("Connecting to ");
  Serial.print(_server);

  // if you get a connection, report back via serial:
  if (!client.connect(_server, _port))
  {
      Serial.println(" fail");
      return;
  }
  else
  {
    Serial.println("Connected to server");
  }
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
    while (client.available())
    {
        String line = client.readStringUntil('\n');
        line.trim();    
        Serial.println(line);    // Uncomment this to show response header
        line.toLowerCase();
        if (line.startsWith("content-length:"))
        {
            contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
        }
        else if (line.length() == 0)
        {
            break;
        }
    }
    return contentLength;
  Serial.println("Content length = "+ String(contentLength));
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
        Serial.println("Cannot beggin update");
    }
}

void otaRoutine()
{
   deviceUpdate();
   //controllerBoardAttributes();
   //downloadControllerFirmware();
}

void deviceUpdate()
{
  static bool prevState = false;
  if(prevBusyState == false)
  {
    bool state = downloadFirmware(DEVICE_OTA);
    prevBusyState=state;
  }
  else
  {
    updateFromFS();
  }
}

void controllerBoardAttributes()
{
    char swurl[100];
    snprintf (iotResource, sizeof(iotResource), "/api/v1/%s/attributes?sharedKeys=fw_version,fw_url",authKey);
    client.stop();
    if(connectApn())
    {
      connectServer(IoTserver,IoTport);
      makeAttributeRequest(String(iotResource));
      readRequest();
      DynamicJsonDocument doc = writeAttributeResponse();
      //String cabUrl = doc["shared"]["sw_url"];
      String shafURL = doc["shared"]["fw_url"];
      
      //String cabVer = doc["shared"]["sw_version"];
      String shafVer = doc["shared"]["fw_version"]; 
      Serial.println(" Shaftver: "+shafVer+" Shafturl: "+shafURL);
      setShaftAttributes(shafURL,shafVer);
      //setcabinAttributes(cabUrl,cabVer);
    }
}

bool downloadFirmware(OTA_device dv)
{
  bool status = false;
  char endPoint[80];
  switch (dv)
  {
  case SHAFT_OTA:
  {
    shaftURL.toCharArray(endPoint, shaftURL.length() + 1);
//    memcpy(endPoint, shaftURL.c_str(), sizeof(shaftURL));
  }
  break;
  case CABIN_OTA:
  {
    memcpy(endPoint, cabinURL.c_str(), sizeof(cabinURL));
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
  Serial.println("endPoint = ");
  //Serial.println("Endpoint is "+String(endPoint));
  for(int i=0;i<(shaftURL.length()+1);i++)
  {
    Serial.print(endPoint[i]);
  }

  if(connectApn())
  {
    connectServer(awsserver, awsport);
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
  return status;
}

int GSMcount()
{
    return pushedMsgCount;
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
    //printPercent(readLength, contentLength);
    if(_num == SHAFT_OTA)
    {
      LittleFS.remove("/fwUpdate.bin");//delete if existing already.
      file = LittleFS.open("/fwUpdate.bin", FILE_APPEND);
      Serial.println("Delete and append shaft file");
    } 
    else if(_num == CABIN_OTA)
    {
      LittleFS.remove("/swUpdate.bin");//delete if existing already.
      file = LittleFS.open("/swUpdate.bin", FILE_APPEND);
      Serial.println("Delete and append cabin file");
    }
    else if(_num == DEVICE_OTA)
    {
      LittleFS.remove("/update.bin");//delete if existing already.
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
		  if (cur_buffer - buffer_ == read_count) 
      {
        Serial.printf("remaining is %d read_count is %d\n",downloadRemaining,read_count);
        file.write(buffer_, read_count);
			//file.print(buffer_, CHUNK_SIZE);
			  cur_buffer = buffer_;
		  }
		}
	}
    file.close();  
}

void setAuthKey(String _authkey)
{
  authKey = _authkey;
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

void  setShaftAttributes(String _fwUrl,String _fwVer)
{
  shaftURL = _fwUrl;
  if(shaftVersion != _fwVer)
  {
    Serial.println("Shaft Version changed");
    Serial.println("shaft version is "+shaftVersion+ " new version is "+_fwVer);
    shaftVersion = _fwVer;
    if(shaftURL!=NULL)
    {
      shaftUpdateAvailable = true;
      addToProcess(_fwVer, SHAFT_NEW_VERSION);
    }
    else
    {
      shaftUpdateAvailable = false; 
    }
    //send message to shaft that update is available
  }
  else
  {
    shaftUpdateAvailable=false;
  }
}

void  setcabinAttributes(String _swUrl, String _swVer)
{
  cabinURL = _swUrl;
  if(cabinVersion != _swVer)
  {
      cabinVersion = _swVer;
      cabinUpdateAvailable = true;
      //send message to cabin that update is available
  }
}

void downloadControllerFirmware()
{
  downloadShaftFirmware();
  //downloadCabinFirmware();
}

void downloadShaftFirmware()
{
  static bool prevShaftState = false;
  switch(shaft_ota)
  {
    case OTA_IDLE:
    {

    }
    break;
    case OTA_POSSIB:
    {

    }
    break;
    case OTA_NOT_NEEDED:
    {
    
    }
    break;
    case OTA_SEND_ACK_GSM:
    {

    }
    break;
    case OTA_SEND_DEC_GSM:
    {

    }
    break;
    case OTA_AWAIT_ACK_GSM:
    {

    }
    break;
    case OTA_SEND_ACK_CABIN:
    {

    }
    break;
    case OTA_SEND_DEC_CABIN:
    {

    }
    break;
    case OTA_COMPLETE_END_CABIN:
    {

    }
    break;
    case OTA_DISCONNECT_ESP_NOW:
    {

    }
    break;
    case OTA_CONNECT_WIFI:
    {

    }
    break;
    case OTA_CONNECT_SERVER:
    {

    }
    break;
    case OTA_DOWNLOAD_FILE:
    {

    }
    break;
    case OTA_UPDATE:
    {

    }
    break;
    case OTA_ERROR:
    {

    }
    break;
    case OTA_COMPLETE:
    {

    }
    break;
    default:
    {
      Serial.println("caught error");
    }
    break;
  }
  if(shaftUpdateAvailable)
  {
    bool state = downloadFirmware(SHAFT_OTA);
    if(prevShaftState!=state)
    {
      prevShaftState=state;
      if(state)
      {
        //send message to shaft that update is downloaded and ready
      }
      else
      {
        Serial.println("Shaft OTA Error occured ");
      }
    }

  }
}

void downloadCabinFirmware()
{
  if(cabinUpdateAvailable)
  {
    bool state = downloadFirmware(CABIN_OTA);
    if(state)
    {
      //send message to shaft that update is downloaded and ready
    }
    else
    {
      Serial.println("cabin OTA Error occured ");
    }
  }
}

void addToProcess(String val, int value)
{
  static String prevVal = "";
  if(prevVal != val)
  {
    prevVal=val;
    Serial1.flush();
    DynamicJsonDocument doc(200);
    doc["type"]=value;
    doc["state"]=val;
    serializeJson(doc,Serial1);  
    //Serial.println("&&&&");
    doc.clear(); 
  }
}