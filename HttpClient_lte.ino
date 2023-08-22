#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "ShaftLog.h"
#include "gsm_module.h"
#include "updateControllers.h"
#include "freertos/FreeRTOS.h"

#include "indicator.h"

String mes="";


TaskHandle_t taskHpHandle = NULL;
void HighPriorityTask(void *args);

void setup() {
  Serial.begin(115200);
  loggerInit();
  gsm_init();
  delay(10);
  xTaskCreate(HighPriorityTask, "tskHP", 2048, NULL, 1, &taskHpHandle);
  Serial.println("Wait...");
  setTimer(millis() + 290*1000);
  Serial.printf("Log and OTA controller : Current version %.1f",getVersion());
}

//loop runs on core 1
void loop() 
{
   //otaTimer(false);
   connectApn();
   otaRoutine();
   sendQueuedDataToGSM();
   updateQueue();
   timeKeeper();
   delay(1);
}

//task runs on core 0
void HighPriorityTask(void *args)
{
  while(1)
  {
     sendReceivedMsgToQueue();
     processLED();
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