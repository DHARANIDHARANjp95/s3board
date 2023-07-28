#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "ShaftLog.h"
#include "gsm_module.h"

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

  Serial.println("Wait...");
}

void loop() 
{
  otaTimer(false);
  // sendQueuedDataToGSM();
  // getUpdateMsg();
  //getData();
}


void sendQueuedDataToGSM()
{
  mes = queueReceivedData();
  if(mes.length() > 0)
  {
    publishSerialData(mes);
  }
}

void getUpdateMsg()
{

}

void HighPriorityTask(void *args)
{
  //initData();
  mes = queueReceivedData();
}
