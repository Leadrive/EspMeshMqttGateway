#define DISABLE_DEBUG_PRINT

#include "bridge_exchange.h"

bool configed = false;
void setup()
{
    serial_init();
    Println("\r\n\r\n========================\r\nStarting...");
    runner = new Scheduler;
    led_init();
    setSerialCallback(execute);
    configed = eepromConfig_init();
    if (!configed)
    {
        Println("No config data !");
        return;
    }
    Println("config data ok");
    bridge_init();
    printf("My node id: %u\r\n", nodeId);
    Println("init process done...\r\n========================\r\n");
}

void loop()
{
    if (!configed)
    {
        return;
    }
    bridge_loop();
    runner->execute();
}