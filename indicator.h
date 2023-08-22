#include "HardwareSerial.h"

typedef enum
{
    TURN_OFF,
    TURN_ON,
    BLINK_TWICE,
    BLINK_FOUR,
}LED_STATE;

void initLED();
void processLED();
void setIndicator(LED_STATE led_state);