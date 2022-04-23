#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include "common.h"
#include <PubSubClient.h>
#include "led.h"

#define WILL_MESSAGE "{\"function\":\"gateway_offline\"}"

WiFiClient wifiClient;
PubSubClient mqttClient;

WiFiEventHandler STAConnected;
WiFiEventHandler STADisconnected;
WiFiEventHandler STAGotIP;

std::function<void(uint32_t, String &)> _mqttReceivedMessageCallback;
std::function<void(char *, unsigned char *, uint32_t)> _mqttReceivedMessageConverterCallback;
bool mqttClientPublish(uint32_t fromNodeId, String &message, bool retained);
void mqttClientConnect();
Task reconnectMqttTask(20000, TASK_FOREVER, mqttClientConnect);

void mqttClient_init()
{
    //设置mqtt
    Println("mqttClient_init...");
    mqttClient.setBufferSize(SERIAL_BUFFER_SIZE);
    mqttClient.setClient(wifiClient);
    mqttClient.setServer(config->mqtt.host, config->mqtt.port);
    uint32_t topicLength = strlen(config->mqtt.subscribe_topic);
    /* mqtt收到订阅消息回调 topic：主题  payload：消息内容起始位置   length：消息内容长度 */
    _mqttReceivedMessageConverterCallback = [&, topicLength](char *topic, unsigned char *payload, uint32_t length) -> void
    {
        if (_mqttReceivedMessageCallback == nullptr)
        {
            DebugPrintln("WARNING: _mqttReceivedMessageCallback = nullptr");
            return;
        }
        char *cleanPayload = (char *)malloc(length + 1);
        memcpy(cleanPayload, payload, length); //截取消息内容部分
        cleanPayload[length] = '\0';           //最后追加0
        String msg(cleanPayload);
        free(cleanPayload);
        String target = String(topic).substring(topicLength - 1); //例如：订阅的主题为Mesh/to/#，从订阅消息“Mesh/to/123456789”，提取对象123456789
        uint32_t target_id = strtoul(target.c_str(), NULL, 10);   // base 代表 str 采用的进制方式，如 base 值为10 则采用10 进制，若 base 值为16 则采用16 进制数等；有非数字转换失败返回0
        //DebugPrintf("Mqtt: received data: '%s',target: %d\r\n", msg.c_str(), target_id);
        _mqttReceivedMessageCallback(target_id, msg);
    };
    mqttClient.setCallback(_mqttReceivedMessageConverterCallback);
    runner->addTask(reconnectMqttTask);
    //开始连接wifi
    WiFi.hostname(config->wifi.host_name);
    WiFi.setAutoReconnect(true);
    std::function<void(const WiFiEventStationModeGotIP &)> stationModeGotIPCallback = [](WiFiEventStationModeGotIP event)
    {
        wifiLed(true);
        Printf("Esp got ip: %s\r\n", event.ip.toString().c_str());
        mqttClientConnect();
    };
    STAGotIP = WiFi.onStationModeGotIP(stationModeGotIPCallback);
    std::function<void(const WiFiEventStationModeDisconnected &)> stationModeDisconnected = [](WiFiEventStationModeDisconnected event)
    {
        Printf("Wifi disconnected! reason: %d\r\n", event.reason);
        wifiLed(false);
    };
    STADisconnected = WiFi.onStationModeDisconnected(stationModeDisconnected);
    WiFi.begin(config->wifi.ssid, config->wifi.password, 6, NULL, true);
}

uint32_t _mqtt_nodeId = 0;

void mqttSetNodeId(uint32_t id)
{
    _mqtt_nodeId = id;
}

void mqttClientConnect()
{
    String clientId(config->mqtt.id);
    clientId += String(random(0xffff), HEX);
    String willTopic(config->mqtt.publish_topic);
    willTopic += _mqtt_nodeId;
    Println("connecting mqtt server...");
    if (mqttClient.connect(clientId.c_str(), config->mqtt.username, config->mqtt.password, willTopic.c_str(), 1, true, WILL_MESSAGE, true)) //尝试连接服务器
    {
        mqttLed(true);
        reconnectMqttTask.disable();
        Println("mqtt server connected successfully!");
        mqttClient.subscribe(config->mqtt.subscribe_topic);
        String message = "{\"function\":\"gateway_info\",\"payload\":{\"gateway_id\":";
        message += _mqtt_nodeId;
        message += "}}";
        mqttClientPublish(_mqtt_nodeId, message, true);
    }
    else
    {
        Println("mqtt server connected failed!");
    }
}

bool mqttClientPublish(uint32_t fromNodeId, String &message, bool retained)
{
    String topic(config->mqtt.publish_topic);
    topic += fromNodeId;
    return mqttClient.publish(topic.c_str(), message.c_str(), retained);
}

void mqttClient_loop()
{
    if (!mqttClient.loop())
    {
        if (!reconnectMqttTask.isEnabled())
        {
            mqttLed(false);
            Printf("mqttClient is offline, state code: %d\r\n", mqttClient.state());
            // checkForErrorReason(mqttClient.state());
            reconnectMqttTask.enable();
            DebugPrintln("reconnectMqttTask enabled");
        }
    }
}

void mqttClientSetReceivedMessageCallback(std::function<void(uint32_t, String &)> cb)
{
    _mqttReceivedMessageCallback = cb;
}

/* void checkForErrorReason(int state)
{
    switch (state)
    {
    case MQTT_CONNECTION_TIMEOUT:
        break;
    case MQTT_CONNECTION_LOST:
        break;
    case MQTT_CONNECT_FAILED:
        break;
    case MQTT_DISCONNECTED:
        break;
    case MQTT_CONNECTED:
        break;
    case MQTT_CONNECT_BAD_PROTOCOL:
        break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
        break;
    case MQTT_CONNECT_UNAVAILABLE:
        break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
        break;
    case MQTT_CONNECT_UNAUTHORIZED:
        break;
    default:
        break;
    }
} */

#endif