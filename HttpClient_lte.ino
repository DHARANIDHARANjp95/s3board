#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "ShaftLog.h"
#include "gsm_module.h"
#include "freertos/FreeRTOS.h"
String mes="";


TaskHandle_t taskHpHandle = NULL;
TaskHandle_t taskMpHandle = NULL;
void HighPriorityTask(void *args);

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  loggerInit();
  gsm_init();
  delay(10);
  xTaskCreate(HighPriorityTask, "tskHP", 2048, NULL, 1, &taskHpHandle);
  Serial.println("Wait...");
}

//loop runs on core 1
void loop() 
{
  // otaTimer(false);
  //getShaftDetails();
  //testJson();
  connectApn();
  sendQueuedDataToGSM();
  updateQueue();
  delay(1);
}


void sendQueuedDataToGSM()
{
  static long int checkInterval = millis();
  if(!emptyQueue())
  {
    //Serial.printf("Shaft msg count = %d GSM push count = %d\n", shaftCount(), GSMcount());
    String newMsg = getFirstMsg();
    if(newMsg.length()>0)
    {
      publishSerialData(newMsg);
      checkInterval=millis();
    }
  }
  else
  {
      if( (millis() - checkInterval > 30*1000))
      {
        checkInterval=millis();
        otaRoutine();
      }
  }
}

//task runs on core 0
void HighPriorityTask(void *args)
{
  while(1)
  {
     sendReceivedMsgToQueue();
     //testData();
    delay(1);
  }
}

void updateQueue()
{
  if(pushChange())
  {
    Serial.println("removing top value");
    popFront();
  }
}

void testJson()
{
  static long int intervalTimer = millis();
  if(millis() - intervalTimer > 1000)
  {
    intervalTimer=millis();
    DynamicJsonDocument doc(250);
    doc["key"]="testing_code";
    doc["type"]=1;
    doc["cabin_floor"]=random(0,4);
    String _msg="";
    serializeJson(doc, _msg);  
    //publishSerialData(_msg);
  }
}