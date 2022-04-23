#ifndef _BRIDGE_EXCHANGE_H_
#define _BRIDGE_EXCHANGE_H_
#include "mesh.h"
#include "mqtt_client.h"

uint32_t nodeId = 0;

void execute(JsonDocument &doc)
{
    const char *function = doc["function"];
    if (!function)
    {
        serial.println("ERROR: No 'function' given!");
        return;
    }
    JsonObject payload = doc["payload"];
    if (!payload)
    {
        serial.println("ERROR: No 'payload' given!");
        return;
    }

    //============================  解析json信息  ===================================

    if (STR_EQUALS(function, "config")) // 网关配置命令
    {
        if (payload.containsKey("system"))
        {
            JsonObject sysConfig = payload["system"];
            parseSystemConfig(sysConfig);
            eepromConfig_commot();
            Println("system config saved, restarting...");
            ESP.restart();
        }
        if (payload.containsKey("app"))
        {
            // appSetToDefaultConfig(true);
            // JsonObject appConfig = payload["app"];
            // parseAppConfig(appConfig);
            // saveAppConfig();
        }
    }
}

void mqttReceivedMessageCallback(uint32_t targetNodeId, String &message)
{
    turnOnMessageLed();
    DebugPrintf("[MQTT ==> Mesh] NodeId = %u, Msg = %s\r\n", nodeId, message.c_str());
    if (targetNodeId == MESH_BROADCAST_ID) //广播
    {
        mesh->sendBroadcast(message); //直接广播
    }
    else if (targetNodeId == nodeId) //自己
    {
        DynamicJsonDocument doc(SERIAL_BUFFER_SIZE * 1.5);
        DeserializationError error = deserializeJson(doc, serial);
        if (error != DeserializationError::Code::Ok)
        {
            Print(F("deserializeJson() failed: "));
            Println(error.f_str());
            return;
        }
        execute(doc);
    }
    else //单播
    {
        mesh->sendSingle(targetNodeId, message); //直接转发
    }
}

void meshReceivedMessageCallback(uint32_t fromNodeId, String &message)
{
    turnOnMessageLed();
    bool result;
    DebugPrintf("[MQTT <== Mesh] NodeId = %u, Msg = %s\r\n", nodeId, message.c_str());
    result = mqttClientPublish(fromNodeId, message, true); //直接转发到mqtt
    if (!result)
    {
        Println("failed to forward message to mqtt!");
    }
}

void bridge_init()
{
    mesh = new Mesh(config->mesh.meshNamePrefix, config->mesh.meshPassword, config->mesh.meshWiFiChannel);
    mesh->setOnMeshReceivedMessageCallback(meshReceivedMessageCallback);
    mesh_init();
    mqttClient_init();
    nodeId = mesh->getNodeId();
    mqttSetNodeId(nodeId);
    mqttClientSetReceivedMessageCallback(mqttReceivedMessageCallback);
    // DebugPrintf("Device class: %u\r\n", device_class);
    // DebugPrintf("Firmware id: %s\r\n", current_firmware_id);
    Printf("Max acceptable message length: %d\r\n", mesh->maxMessageLength());
    randomSeed(nodeId);
}

void bridge_loop()
{
    mesh->performMaintenance();
    mqttClient_loop();
}

/*
IPAddress myIP(0, 0, 0, 0);
IPAddress getlocalIP();

void bridge_loop()
{
    if (myIP != getlocalIP())
    {
        myIP = getlocalIP();
        DebugPrintln("INFO: bridge ip is: " + myIP.toString());
        if (mqttClientConnect())
        {
            mqttClient.publish("painlessMesh/from/gateway", "Ready!");
            mqttClient.subscribe("painlessMesh/to/#");
        }
    }
} */

#endif