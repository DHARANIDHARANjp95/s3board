#include "ArduinoJson.h"


typedef enum{
    SEQUENCE_BEGIN,
    LIFT_STATE,
    SHAFT_VERSION,
    WIFI_PASSWORD,
    WIFI_USERID,
    SET_FLOOR,
}LOG_SEQUENCE;

typedef enum{
    COMM_IDLE,
    COMM_WAIT_FOR_MESSAGE,
    COMM_REQUEST_INITIAL_DATA,
    COMM_SEND_INITIAL_DATA,
    COMM_SEND_RECEIVED_DATA,
    COMM_STOP
}comm_state;

typedef enum
{
    CALL_BOOKED,
    CALL_CLEARED,
    CALL_FAILED,
    CURRENT_FLOOR,
    ERROR_TYPE,
    LOCK_STATUS,
    CUSTOMER_NAME,
    MOTOR_COUNT,
    EV_STATUS,
    LIDAR_VALUE,
    OVERLOAD_SENSOR,
    CALIBRATION_STATUS,
    DOWNFLOOR_CALL,
    UPFLOOR_CALL,
    SAMEFLOOR_CALL,
    READY_FOR_UPDATE
}DATAPOINT_DP;


typedef enum
{
    SHAFT_READY,
    SHAFT_BUSY,
    CABIN_READY,
    CABIN_BUSY
}UPDATE_sts;

typedef enum
{
  AUTO_FLR_STS_PROCESS_DONE,
  AUTO_FLR_STS_PROCESS_ERROR,
  AUTO_FLR_STS_COMPLETE,
  AUTO_FLR_STS_BUSY,
  AUTO_FLR_STS_ERROR
}AUTO_FLR_STS_em;

typedef enum
{
  CALL_SERVE_STS_NOFAIL,
  CALL_SERVE_STS_DOORLOCK_FAIL,
  CALL_SERVE_STS_SAFETY_FAILURE,
  CALL_SERVE_CAN_STS_SAFETY_FAILURE,
  //CALL_SERVE_OVER_STS_SAFETY_FAILURE,
  CALL_SERVE_STS_LL_FAIL,
  CALL_SERVE_STS_LIDAR_FAIL,
  CALL_SERVE_STS_CAB_DERAIL,
  CALL_SERVE_STS_OVER_LOAD,
  CALL_SERVE_STS_OVER_SPEED,
  CALL_SERVE_STS_POWER_FAILURE,
  CALL_SERVE_STS_ML_FAIL_BEFORE_CALL,
  // CALL_SERVE_STS_LOCK_RUN_FAIL,
  CALL_SERVE_STS_ESP_COM_FAILURE,
  CALL_SERVE_STS_COMPLETE
} CALL_SERVE_STS;

typedef enum
{
  GND_FLOOR,
  FIRST_FLOOR,
  SECOND_FLOOR,
  THIRRD_FLOOR,
  FOURTH_FLOOR,
  FIVTH_FLOOR,
  SEVENTH_FLOOR,
} FLOOR_NUM_em;
void initData();
String queueReceivedData();
int sendMSgToShaft(LOG_SEQUENCE log_st, int value);
void loggerInit();