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
  xTaskCreate(HighPriorityTask, "tskHP", 2048, NULL, 2, &taskHpHandle);
  Serial.println("Wait...");
}

//loop runs on core 1
void loop() 
{
  // otaTimer(false);
   connectApn();
   sendQueuedDataToGSM();
   updateQueue();
   delay(1);
  //getData();
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
    testData();
    delay(1);
  }
}

void updateQueue()
{
  if(pushChange())
  {
    popFront();
  }
}