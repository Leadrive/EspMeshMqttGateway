#ifndef _EEPROM_CONFIG_H_
#define _EEPROM_CONFIG_H_
#include "common.h"
#include <EEPROM.h>
#include <ArduinoJson.h>

struct EEPROMConfig
{
    uint32_t validity_1;
    struct MeshConfig
    {
        char meshNamePrefix[32];
        char meshPassword[32];
        uint8_t meshWiFiChannel;
    } mesh;
    struct WifiConfig
    {
        char ssid[32];
        char password[32];
        char host_name[16];
    } wifi;
    struct MqttConfig
    {
        char host[32];
        uint32_t port;
        char id[32];
        char username[32];
        char password[32];
        char subscribe_topic[32];
        char publish_topic[32];
    } mqtt;
    uint32_t validity_2;
} * config;

void parseSystemConfig(JsonObject &payload)
{
    if (!payload)
    {
        Printf("sys config payload is null");
        return;
    }
    JsonObject mesh = payload["mesh"];
    if (mesh)
    {
        const char *meshNamePrefix = mesh["name_prefix"];
        strcpy(config->mesh.meshNamePrefix, meshNamePrefix);
        const char *meshPassword = mesh["password"];
        strcpy(config->mesh.meshPassword, meshPassword);
        config->mesh.meshWiFiChannel = mesh["wifi_channel"];
    }
    JsonObject payload_wifi_cfg = payload["wifi"];
    if (payload_wifi_cfg)
    {
        const char *payload_wifi_cfg_ssid = payload_wifi_cfg["ssid"];
        strcpy(config->wifi.ssid, payload_wifi_cfg_ssid);
        const char *payload_wifi_cfg_password = payload_wifi_cfg["password"];
        strcpy(config->wifi.password, payload_wifi_cfg_password);
        const char *payload_wifi_cfg_host_name = payload_wifi_cfg["host_name"];
        strcpy(config->wifi.host_name, payload_wifi_cfg_host_name);
    }
    JsonObject payload_mqtt_cfg = payload["mqtt"];
    if (payload_mqtt_cfg)
    {
        const char *payload_mqtt_cfg_host = payload_mqtt_cfg["host"];
        strcpy(config->mqtt.host, payload_mqtt_cfg_host);
        int payload_mqtt_cfg_port = payload_mqtt_cfg["port"];
        config->mqtt.port = payload_mqtt_cfg_port;
        const char *payload_mqtt_cfg_id = payload_mqtt_cfg["id"];
        strcpy(config->mqtt.id, payload_mqtt_cfg_id);
        const char *payload_mqtt_cfg_username = payload_mqtt_cfg["username"];
        strcpy(config->mqtt.username, payload_mqtt_cfg_username);
        const char *payload_mqtt_cfg_password = payload_mqtt_cfg["password"];
        strcpy(config->mqtt.password, payload_mqtt_cfg_password);
        const char *payload_mqtt_cfg_sub_tpc = payload_mqtt_cfg["subscribe_topic"];
        strcpy(config->mqtt.subscribe_topic, payload_mqtt_cfg_sub_tpc);
        const char *payload_mqtt_cfg_pub_tpc = payload_mqtt_cfg["publish_topic"];
        strcpy(config->mqtt.publish_topic, payload_mqtt_cfg_pub_tpc);
    }
}

bool eepromConfig_commot()
{
    config->validity_1 = 0x55aa;
    config->validity_2 = 0xaa55;
    return EEPROM.commit();
}

bool eepromConfig_init()
{
    EEPROM.begin(sizeof(EEPROMConfig));
    config = (EEPROMConfig *)EEPROM.getDataPtr();
    return (config->validity_1 == 0x55aa && config->validity_2 == 0xaa55); //检查数据配置格式
}

#endif