#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "ShaftLog.h"

#define TINY_GSM_MODEM_SIM7600
#define SerialMon Serial  

#define TINY_GSM_RX_BUFFER 650

const char coords[] = "/api/v1/testing_code/telemetry";
#define TINY_GSM_USE_GPRS true

// Your GPRS credentials, if any
const char apn[]      = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details
const char server[]   = "13.126.126.206";
const int  port       = 8080;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsm        modem(Serial1);

TinyGsmClient client(modem);
HttpClient    http(client, server, port);

TaskHandle_t taskHpHandle = NULL;
TaskHandle_t taskMpHandle = NULL;
void HighPriorityTask(void *args);

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  Serial1.begin(115200,SERIAL_8N1,16,17);
  delay(10);

  Serial.println("Wait...");

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);
  xTaskCreate(HighPriorityTask, "tskHP", 2048, NULL, 1, &taskHpHandle);
}

void loop() {
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(200);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }
  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }

  publishData(1.3f, 2.4f);
  getData();
}


void publishData(float lat, float lon)
{
  char latit[] = "lat";
  char longi[] = "lon";
  // Prepare JSON document
  StaticJsonDocument<200> doc;
  doc[latit] = lat;
  doc[longi] = lon;
  // Serialize JSON document
  String json;
  serializeJson(doc, json);
  http.beginRequest();
  Serial.println("Json value ="+json);
  int err = http.post(coords, "application/json", json);
  if (err != 0)
  {
    SerialMon.println(F("failed to connect"));
    delay(10000);
    return;
  }
  else
  {
    Serial.println("connected");
  }
  int status = http.responseStatusCode();
  Serial.printf("status code= %d\n",status);
  // String body = http.responseBody();
  // SerialMon.println(F("Response:"));
  // SerialMon.println(body);
  //http.stop();
  SerialMon.println(F("Server disconnected"));
}

void getData()
{
  SerialMon.print(F("Performing HTTP GET request... "));
  int err = 0;// http.get(resource);
  if (err != 0) 
  {
    SerialMon.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  SerialMon.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName  = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    SerialMon.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    SerialMon.print(F("Content length is: "));
    SerialMon.println(length);
  }
  if (http.isResponseChunked()) {
    SerialMon.println(F("The response is chunked"));
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


void HighPriorityTask(void *args)
{
  initData();
  queueReceivedData();
}