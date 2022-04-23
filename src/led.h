#ifndef _LED_H_
#define _LED_H_

#include "common.h"

#define MQTT_LED 12
#define WIFI_LED 13
//#define MESSAGE_LED 14
#define MESSAGE_LED LED_BUILTIN

void messageLed(bool on)
{
    digitalWrite(MESSAGE_LED, !on);
}

Task msgLedDelay(250, TASK_FOREVER,
                 []()
                 {
                     messageLed(false);
                     msgLedDelay.disable();
                 });
                 
void turnOnMessageLed()
{
    messageLed(true);
    msgLedDelay.enable();
}

void mqttLed(bool on)
{
    digitalWrite(MQTT_LED, !on);
}

void wifiLed(bool on)
{
    digitalWrite(WIFI_LED, !on);
}

void led_init()
{
    pinMode(MESSAGE_LED, OUTPUT);
    messageLed(false);
    pinMode(MQTT_LED, OUTPUT);
    mqttLed(false);
    pinMode(WIFI_LED, OUTPUT);
    wifiLed(false);
    runner->addTask(msgLedDelay);
}
#endif