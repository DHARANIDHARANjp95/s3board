#include "gsm_module.h"
#include <ArduinoJson.h>
#define TINY_GSM_MODEM_SIM7600
#define SerialMon Serial  

#define TINY_GSM_RX_BUFFER 650


#define TINY_GSM_USE_GPRS true

// Your GPRS credentials, if any
const char apn[]      = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char user[] = "";
const char pass[] = "";
// Server details
const char IoTserver[]   = "13.126.126.206";
const char server[] = "esp32otabalajitest.s3.ap-south-1.amazonaws.com";
const int  IoTport       = 8080;
const int port = 80;
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsm        modem(Serial2);

TinyGsmClient client(modem);
HttpClient    http(client, IoTserver, IoTport);

String message="";

String makerId = "ASM_S3_PRO_1";
String sNo = "factoryg3";
String controllerType="ltemodule";
char urlResource[80];
char shaftResource[80];
char cabinresource[80];

#define BLINK_LED 2

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

void gsm_init()
{
    Serial2.begin(115200);
    pinMode(BLINK_LED,OUTPUT);
    Serial.println("Initializing modem...");
    modem.restart();
    String modemInfo = modem.getModemInfo();
    Serial.print("Modem Info: ");
    Serial.println(modemInfo);
    //connectInternet();
    snprintf (urlResource, sizeof(urlResource), "/ASM_S3_PRO_1/%s/%s/%.1f/lte.bin", controllerType, sNo, next_version);
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
  http.get(urlResource);
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
  DynamicJsonDocument doc(250);
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
  String authKey = doc["key"];
  String msgtype="";
  int typeVal = doc["type"];
  if(typeVal == ATTRIBUTE)
  {
    msgtype="attributes";
  }
  else
  {
    msgtype="telemetry";
  }
  //msgtype="telemetry";
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

void initSpiffs()
{
  if (!LittleFS.begin(true))
  {
      Serial.println("LittleFS Mount Failed");
      LittleFS.format();
  }
}

void connectAWSServer()
{
  Serial.print("Connecting to ");
  Serial.print(server);

  // if you get a connection, report back via serial:
  if (!client.connect(server, port))
  {
      Serial.println(" fail");
      return;
  }
  else
  {
    Serial.println("Connected to server");
  }
}


void makeRequest()
{
  // Make a HTTP request:
  client.print(String("GET ") + String(urlResource) + " HTTP/1.0\r\n");
  client.print(String("Host: ") + server + "\r\n");
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
        //Serial.println(line);    // Uncomment this to show response header
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

void writeResponse()
{
    timeout = millis();
    uint32_t readLength = 0;
    unsigned long timeElapsed = millis();
    //printPercent(readLength, contentLength);
    LittleFS.remove("/update.bin");
    file = LittleFS.open("/update.bin", FILE_APPEND);
    while (readLength < contentLength && client.connected() && millis() - timeout < 10000L)
    {
        int i = 0;
        while (client.available())
        {
                // read file data to LittleFS
            if (!file.print(char(client.read())))
            {
                //Serial.println("Appending file");
            }
            readLength++;

            if (readLength % (contentLength / 13) == 0)
            {
                printPercent(readLength, contentLength);
            }
            timeout = millis();
        }
    }

    file.close();
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
  if(connectApn())
  {
    connectAWSServer();
    makeRequest();
    if(readRequest())
    {
      writeResponse(); 
      updateFromFS();  
    } 
    else
    {
      Serial.println("empty content");
    } 
  }
}

// void otaTimer(bool state)
// {
//     if (prevBusyState!=state)
//     {
//         //if state = false, idle state start otatimer
//         if(!state)
//         {
//             otaStatus=true; 
//         }
//         else
//         {
//             otaStatus=false;
//             otaTimerStart=false;
//         }
//         //if state = true, busy, stop and reset timer.
//     }
//     prevBusyState=state;
//     if(otaStatus)
//     {
//       otaTimerStart = true;
//       otaStatus=false;
//       otaDisableTime = millis(); //
//       printf("*************************OTA check after %d minutes*************************\n", otaCheck);
//     }
//     else
//     {
//       if(otaTimerStart)
//       {
//         if(tickDiff(otaDisableTime,millis()) > otaDelay)
//         {
//           otaTimerStart=false;
//           otaStatus=true;
//           printf("*************************OTA check begins*************************\n");
//           otaRoutine();
//         }
//       }
//     }
// }


void otaTimer(bool state)
{
    static long int timerV = millis();
    if(millis() - timerV > 30*1000)
    {   
        otaRoutine();
        timerV = millis();
    }
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