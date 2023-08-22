#include "indicator.h"

#define BLINK_LED 2

void initLED()
{
    pinMode(BLINK_LED,OUTPUT);
}

LED_STATE led_em;
long int timer = millis();
void processLED()
{
    switch (led_em)
    {
        case TURN_OFF:
        {
            digitalWrite(BLINK_LED, LOW);
        }
        break;
        case TURN_ON:
        {
            digitalWrite(BLINK_LED, HIGH);
        }
        break;
        case BLINK_TWICE:
        {
            static bool state = LOW;
            if(millis() - timer > 500)
            {
                state = !state;
                digitalWrite(BLINK_LED, state);
            }
        }
        break;
        case BLINK_FOUR:
        {
            static bool state = LOW;
            if(millis() - timer > 250)
            {
                state = !state;
                digitalWrite(BLINK_LED, state);
            }
        }
        break;
        default:
        break;
    }
}

void setIndicator(LED_STATE led_state)
{
    led_em = led_state;
    timer = millis();
}