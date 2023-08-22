#include <HardwareSerial.h>
#include "ShaftLog.h"
#include "common.h"
bool shaftCommEstablished = false;
#include <iostream>
#include <queue>
#include "gsm_module.h"

#include <ArduinoJson.h>
#include <cJSON.h>

#include <bits/stdc++.h>
using namespace std;

struct product {
    int x;
    char* y;

    product (int i, char j[50]) : x(i), y(j) {}
} ;

queue<product> shaftData;
queue<String> shaftMsg;

bool receivedMsg = false;

String postMsg = "";

#define SERIAL1_RXPIN 25
#define SERIAL1_TXPIN 26


/****************************************/
bool onceChecked = true;
/****************************************/

/*****************************************/
#define HEART_BEAT_INTERVAL 300*1000
long int heartBeatDuration = 0;

#define INACTIVE_PERIOD 60*1000
long int inactiveMaxDuration = 0;

/*****************************************/
void loggerInit()
{
    Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
}

String queueReceivedData()
{
    String s = "";
    if(Serial1.available())
    {
        s = Serial1.readStringUntil('}');
        s=s+"}";
        Serial.println("received message = "+s);
     }
    return s;
}

void sendReceivedMsgToQueue()
{
    static String prevMsg = "";
    String recvMsg =  queueReceivedData(); 
     if(recvMsg.length()>2) //excluding curly braces
     {
        if(prevMsg != recvMsg)
        {
            parseData(recvMsg);
            prevMsg = recvMsg;
            inactiveMaxDuration = millis();
        }
     }
}

void getShaftDetails()
{
    DynamicJsonDocument doc(60);
    doc["state"]="true";
    doc["type"]=1;
    String _msg="";
    serializeJson(doc, Serial1); 
    doc.clear();     
}

void setTimer(uint32_t timer)
{
    heartBeatDuration = timer;
}
void setInactiveTimer(uint32_t timer)
{
    inactiveMaxDuration = timer;
}

void timeKeeper()
{
    if ((tickDiff(heartBeatDuration, millis())> HEART_BEAT_INTERVAL))
    {
        Serial.println("need shaft data");
        setTimer(millis());
        getShaftDetails();
    }

    if(tickDiff(inactiveMaxDuration, millis())> INACTIVE_PERIOD)
    {
        setInactiveTimer(millis());
        Serial.println("Shaft is inactive");
    }
}