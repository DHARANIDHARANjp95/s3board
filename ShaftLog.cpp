#include <HardwareSerial.h>
#include "ShaftLog.h"
bool shaftCommEstablished = false;
#include <iostream>
#include <queue>

using namespace std;
queue<uint8_t> statemsg, datamsg;
queue<String> shaftMsg;

LOG_SEQUENCE log_sq;
comm_state cm_st;
DATAPOINT_DP dt_dp;

bool receivedMsg = false;

String postMsg = "";
void sendTelemetry(DATAPOINT_DP state, DATAPOINT_DP data);
void sendAttributes(DATAPOINT_DP state, DATAPOINT_DP data);

#define SERIAL1_RXPIN 25
#define SERIAL1_TXPIN 26

int queuedMsgCount = 0;
/****************************************/
bool onceChecked = true;
/****************************************/
void pushMsg(String locl);
void loggerInit()
{
    Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
}
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
        // if(sendMSgToShaft(SEQUENCE_BEGIN, 1))
        // {
        //     cm_st = COMM_REQUEST_INITIAL_DATA;
        // }
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
// void queueReceivedData()
// {
//     int dataMode = 0;
//     DATAPOINT_DP state;
//     DATAPOINT_DP data;
//     if(Serial1.available())
//     {
//         String s = Serial1.readStringUntil('\n');
//         DeserializationError error = deserializeJson(doc, s);
//         if (error) 
//         {
//            // 
//         }
//         else
//         {
//             dataMode = doc["type"];
//             state = doc["state"];
//             data = doc["data"];
//             switch (dataMode)
//             {
//             case CALL_BOOKED:
//             {
//                 switch (state)
//                 {
//                     case GND_FLOOR:
//                     {
//                         postMsg = "Ground Floor Booked";
//                     }
//                     break;
//                     case FIRST_FLOOR:
//                     {
//                         postMsg = "First Floor Booked";
//                     }
//                     break;
//                     case SECOND_FLOOR:
//                     {
//                         postMsg = "Second Floor Booked";
//                     }
//                     break;
//                     case THIRRD_FLOOR:
//                     {
//                         postMsg = "Third Floor Booked";
//                     }
//                     break;
//                     default:
//                         break;
//                 }
//             }break;
//             case CALL_CLEARED:
//             {
//                 switch (data)
//                 {
//                     case GND_FLOOR:
//                     {
//                         postMsg = "Ground Floor cleared";
//                     }
//                     break;
//                     case FIRST_FLOOR:
//                     {
//                         postMsg = "First Floor cleared";
//                     }
//                     break;
//                     case SECOND_FLOOR:
//                     {
//                         postMsg = "Second Floor cleared";
//                     }
//                     break;
//                     case THIRRD_FLOOR:
//                     {
//                         postMsg = "Third Floor cleared";
//                     }
//                     break;
//                     default:
//                         break;
//                 }
//             }break;
//             case CALL_FAILED:
//             {
//                 switch (state)
//                 {
//                     case GND_FLOOR:
//                     {
//                         postMsg = "Ground Floor failed";
//                     }
//                     break;
//                     case FIRST_FLOOR:
//                     {
//                         postMsg = "First Floor failed";
//                     }
//                     break;
//                     case SECOND_FLOOR:
//                     {
//                         postMsg = "Second Floor failed";
//                     }
//                     break;
//                     case THIRRD_FLOOR:
//                     {
//                         postMsg = "Third Floor failed";
//                     }
//                     break;
//                     default:
//                         break;
//                 }
//             }break;
//             case CURRENT_FLOOR:
//             {
//                 switch (state)
//                 {
//                     case GND_FLOOR:
//                     {
//                         postMsg = "Lift at Ground Floor";
//                     }
//                     break;
//                     case FIRST_FLOOR:
//                     {
//                         postMsg = "Lift at First Floor";
//                     }
//                     break;
//                     case SECOND_FLOOR:
//                     {
//                         postMsg = "Lift at Second Floor";
//                     }
//                     break;
//                     case THIRRD_FLOOR:
//                     {
//                         postMsg = "Lift at Third Floor";
//                     }
//                     break;
//                     default:
//                         break;
//                 }
//             }break;
//             case ERROR_TYPE:
//             {
//                 switch (state)
//                 {
//                 case   CALL_SERVE_STS_NOFAIL:
//                 {
//                         postMsg ="Call Serving without fail";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_DOORLOCK_FAIL:
//                 {   
//                         postMsg = "Door Lock error";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_SAFETY_FAILURE:
//                 {
//                         postMsg = "Lock error";
//                 }
//                 break;
//                 case    CALL_SERVE_CAN_STS_SAFETY_FAILURE:
//                 {
//                         postMsg = "CAN Error";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_LL_FAIL:
//                 {
//                         postMsg = "Landing Lever Fail";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_LIDAR_FAIL:
//                 {
//                         postMsg = "LIDAR error";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_CAB_DERAIL:
//                 {
//                         postMsg = "Cabin Derail Error";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_OVER_LOAD:
//                 {
//                         postMsg = "OV Error";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_OVER_SPEED:
//                 {
//                         postMsg = "Speed Error";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_POWER_FAILURE:
//                 {
//                         postMsg = "Power Failure Error";
//                 }   
//                 break;
//                 case    CALL_SERVE_STS_ML_FAIL_BEFORE_CALL:
//                 {
//                         postMsg = "Lock Error";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_ESP_COM_FAILURE:
//                 {
//                         postMsg = "Communication Failure";
//                 }
//                 break;
//                 case    CALL_SERVE_STS_COMPLETE:
//                 {
//                         postMsg = "Call Serve Complete";
//                 }
//                 break;                
//                 default:
//                 break;
//                 }

//             }break;
//             case LOCK_STATUS:
//             {
                
//             }break;
//             case CUSTOMER_NAME:
//             {

//             }break;
//             case MOTOR_COUNT:
//             {
//                 postMsg = "Motor running "+String();    
//             }
//             break;
//             case EV_STATUS:
//             {
//                 if(state)
//                 {   
//                     postMsg = "EV On";
//                 }
//                 else
//                 {
//                     postMsg = "EV Off";
//                 }
//             }break;
//             case LIDAR_VALUE:
//             {
//                 postMsg = "Cabin is at "+String(state)+"cm";   
//             }break;
//             case OVERLOAD_SENSOR:
//             {
//                 if(state)
//                 {   
//                     postMsg = "Overload On";
//                 }
//                 else
//                 {
//                     postMsg = "Overload Off";
//                 }
//             }break;
//             case CALIBRATION_STATUS:
//             {
//                 switch (state)
//                 {
//                     case AUTO_FLR_STS_PROCESS_DONE:
//                     {
//                         postMsg = "Autocalibration Process Done";
//                     }
//                     break;
//                     case AUTO_FLR_STS_PROCESS_ERROR:
//                     {
//                         postMsg = "Autocalibration Process Error";
//                     }
//                     break;
//                     case  AUTO_FLR_STS_COMPLETE:
//                     {
//                         postMsg = "Autocalibration Process Complete";
//                     }
//                     break;
//                     case AUTO_FLR_STS_BUSY:
//                     {
//                         postMsg = "Autocalibration Process busy";    
//                     }
//                     break;
//                     case AUTO_FLR_STS_ERROR:
//                     {
//                         postMsg = "Autocalibration Status Error";    
//                     }
//                     break;
//                     default:
//                     break;
//                 }

//             }break;
//             case DOWNFLOOR_CALL:
//             {
//                 switch (state)
//                 {
//                     case GND_FLOOR:
//                     {
//                         postMsg = "Downcall to Ground Floor";
//                     }    
//                     break;
//                     case FIRST_FLOOR:
//                     {
//                         postMsg = "Downcall to First Floor";
//                     }
//                     break;
//                     case SECOND_FLOOR:
//                     {
//                         postMsg = "Downcall to Second Floor";
//                     }
//                     break;
//                     case THIRRD_FLOOR:
//                     {
//                         postMsg = "Downcall to Third Floor";
//                     }
//                     break;
//                     default:
//                     break;
//                 }
//             }break;
//             case UPFLOOR_CALL:
//             {
//                 switch (state)
//                 {
//                     case GND_FLOOR:
//                     {
//                         postMsg = "Upcall to Ground Floor";
//                     }    
//                     break;
//                     case FIRST_FLOOR:
//                     {
//                         postMsg = "Upcall to First Floor";
//                     }
//                     break;
//                     case SECOND_FLOOR:
//                     {
//                         postMsg = "Upcall to Second Floor";
//                     }
//                     break;
//                     case THIRRD_FLOOR:
//                     {
//                         postMsg = "Upcall to Third Floor";
//                     }
//                     break;
//                     default:
//                     break;
//                 }
//             }break;
//             case SAMEFLOOR_CALL:
//             {
//                 switch (state)
//                 {
//                     case GND_FLOOR:
//                     {
//                         postMsg = "Same call to Ground Floor";
//                     }    
//                     break;
//                     case FIRST_FLOOR:
//                     {
//                         postMsg = "Same call to First Floor";
//                     }
//                     break;
//                     case SECOND_FLOOR:
//                     {
//                         postMsg = "Same call to Second Floor";
//                     }
//                     break;
//                     case THIRRD_FLOOR:
//                     {
//                         postMsg = "Same call to Third Floor";
//                     }
//                     break;
//                     default:
//                     break;
//                 }
//             }break;
//             case READY_FOR_UPDATE:
//             {
//                 switch (state)
//                 {
//                     case SHAFT_READY:
//                     {
//                         postMsg = "Shaft ready for update";
//                     }    
//                     break;
//                     case SHAFT_BUSY:
//                     {
//                         postMsg = "Shaft can not update";
//                     }
//                     case CABIN_READY:
//                     {
//                         postMsg = "Cabin ready for update";
//                     }
//                     case CABIN_BUSY:
//                     {
//                         postMsg = "Cabin can not update";
//                     }
//                     break;
//                     default:
//                     break;
//                 }
//             }
//             break;
//             default:
//             break;
//             }
//             //addtoqueue();
//         }
//     }
// }


int sendMSgToShaft(LOG_SEQUENCE log_st, int value)
{
    Serial1.println("{\"sequence_begin\":true}");
    return 0;
}

void sendTelemetry(DATAPOINT_DP state, DATAPOINT_DP data)
{

}
void sendReceivedMsgToQueue()
{
    static String prevMsg = "";
    String recvMsg =  queueReceivedData(); 
     if(recvMsg.length()>2) //excluding curly braces
     {
        if(recvMsg.indexOf("version")>-1)
        {
            //version is available
            // StaticJsonBuffer<200> doc;
            // deserializeJson(doc, recvMsg);
        }
        else
        {
            if(prevMsg != recvMsg)
            {
                Serial.println("received msg after comparing = "+recvMsg);
                pushMsg(recvMsg);
                prevMsg = recvMsg;
            }
        }
     }


}

int shaftCount()
{
    return queuedMsgCount;
}

String getFirstMsg()
{
    return shaftMsg.front();
}

void popFront()
{
    shaftMsg.pop();
    Serial.println("Popped the first one");
}

void testData()
{
    static long int timerin = millis();
    
    if(onceChecked && (millis() - timerin > 15*1000))
    {
        timerin=millis();
        onceChecked=false;
        for(int i=0;i<5;i++)
        {
            DynamicJsonDocument doc(250);
            doc["key"]="testing_code";
            doc["type"]=0;
            doc["cabin_floor"]=random(0,4);
            String _msg="";
            serializeJson(doc, _msg);  
            pushMsg(_msg);
            // JsonObject object = doc.as<JsonObject>();
            // object.remove("type");
            // object.remove("key");
            // _msg="";
            // serializeJson(doc, _msg);
        }
    }
}

void pushMsg(String locl)
{
    shaftMsg.push(locl);
    queuedMsgCount++;
}

bool emptyQueue()
{
    return shaftMsg.empty();
}


/// @brief 
void getShaftDetails()
{
    static long int interval = millis();
    if(!receivedMsg && (millis() - interval > 5000))
    {
        DynamicJsonDocument doc(250);
        doc["state"]="shaft_attributes";
        String _msg="";
        serializeJson(doc, Serial1); 
        doc.clear();       
    }
}
