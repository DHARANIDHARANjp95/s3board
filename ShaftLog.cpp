#include <HardwareSerial.h>
#include "ShaftLog.h"
bool shaftCommEstablished = false;
#include <iostream>
#include <queue>
using namespace std;
queue<uint8_t> statemsg, datamsg;

LOG_SEQUENCE log_sq;
comm_state cm_st;
DATAPOINT_DP dt_dp;


void sendTelemetry(DATAPOINT_DP state, DATAPOINT_DP data);
void sendAttributes(DATAPOINT_DP state, DATAPOINT_DP data);
void initData()
{
    comm_state cm_st = COMM_IDLE;
    switch (cm_st)
    {
    case COMM_IDLE:
    {
        cm_st = COMM_WAIT_FOR_MESSAGE;
    }
    break;
    case COMM_WAIT_FOR_MESSAGE:
    {
        if(sendMSgToShaft(SEQUENCE_BEGIN, 1))
        {
            cm_st = COMM_REQUEST_INITIAL_DATA;
        }
    }
    break;
    case COMM_REQUEST_INITIAL_DATA:
    {
        if(true)
        {
            cm_st = COMM_SEND_INITIAL_DATA;
        }
    }
    break;
    case COMM_SEND_INITIAL_DATA:
    {

    }
    break;
    case COMM_STOP:
    {
        shaftCommEstablished = true;
    }
    break;
    default:
        break;
    }
}

void queueReceivedData()
{
    DynamicJsonDocument doc(500);
    int dataMode = 0;
    DATAPOINT_DP state;
    DATAPOINT_DP data;
    if(Serial1.available())
    {
        String s = Serial1.readStringUntil('\n');
        DeserializationError error = deserializeJson(doc, s);
        if (error) 
        {
           // 
        }
        else
        {
            dataMode = doc["type"];
            state = doc["state"];
            data = doc["data"];
            switch (state)
            {
            case /* constant-expression */:
                /* code */
                break;
            
            default:
                break;
            }
            //addtoqueue();
        }
    }
}


int sendMSgToShaft(LOG_SEQUENCE log_st, int value)
{
    Serial1.println("{\"sequence_begin\":true}");
}

void sendTelemetry(DATAPOINT_DP state, DATAPOINT_DP data)
{

}