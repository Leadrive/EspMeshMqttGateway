#ifndef _ESP_WIFI_H_
#define _ESP_WIFI_H_
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFI.h>
#endif
#include <ArduinoJson.h>
#include "serial.h"

void wifi_ready_resp();

void wifiEvent(WiFiEvent_t event) // wifi事件回调函数
{

  switch (event)
  {
#ifdef ESP32
  case SYSTEM_EVENT_WIFI_READY:
    serial.println(" ESP WiFi ready");
    break;
  case SYSTEM_EVENT_SCAN_DONE:
    serial.println("ESP finish scanning AP");
    break;
  case SYSTEM_EVENT_STA_START:
    serial.println("ESP station start");
    break;
  case SYSTEM_EVENT_STA_STOP:
    serial.println("ESP station stop");
    break;
#endif
#ifdef ESP32
  case SYSTEM_EVENT_STA_CONNECTED:
#else
  case WIFI_EVENT_STAMODE_CONNECTED:
#endif
    serial.println("ESP station connected to AP");
    break;
#ifdef ESP32
  case SYSTEM_EVENT_STA_DISCONNECTED:
#else
  case WIFI_EVENT_STAMODE_DISCONNECTED:
#endif
    serial.println("ESP station disconnected from AP");
    // TODO:消息通知
    break;
#ifdef ESP32
  case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    serial.println("the auth mode of AP connected by ESP32 station changed");
    break;
#endif
#ifdef ESP32
  case SYSTEM_EVENT_STA_GOT_IP:
#else
  case WIFI_EVENT_STAMODE_GOT_IP:
#endif
    serial.println("ESP station got IP from connected AP");
    wifi_ready_resp();
    break;
#ifdef ESP32
  case SYSTEM_EVENT_STA_LOST_IP:
    serial.println("ESP station lost IP and the IP is reset to 0");
    break;
#endif
  default:
    serial.println(event);
    break;
  }
}

//{"function":"wifi_ready","payload":{"ip":"192.168.100.100"}}
void wifi_ready_resp()
{
  IPAddress ip = WiFi.localIP();
  serial.print("ESP client IP is: ");
  serial.println(ip);
  StaticJsonDocument<128> respDoc;
  respDoc["function"] = "wifi_ready";
  JsonObject payload = respDoc.createNestedObject("payload");
  payload["ip"] = ip.toString();
  serializeJson(respDoc, serial_json_out);
}

void wifi_init(JsonObject &payload)
{
  const char *ssid = payload["ssid"];
  const char *password = payload["password"];
  const char *hostName = payload["host_name"];
  serial.printf("Connecting Wifi '%s' ", ssid);
  WiFi.setHostname(hostName);
  WiFi.setAutoReconnect(true);
  WiFi.onEvent(wifiEvent);
  WiFi.begin(ssid, password);
  /* int time_count = 0;
  while (true)
  {
    wl_status_t status = WiFi.status();
    serial.println(status);
    if (status == WL_CONNECTED)
    {
      serial.println("\r\nConnected successfully");
      WiFi.localIP();
      break;
    }
    else if (status == WL_NO_SSID_AVAIL)
    {
      serial.println("\r\nNo ssid avail");
      break;
    }
    delay(500);
    serial.print('.');
    if (time_count > 60)
    {
      serial.println("\r\nConnection failed");
      break;
    }
  } */
}

void wifi_disconnect(JsonObject &payload)
{
  if (WiFi.disconnect())
  {
    serial.println("Wifi disconnected");
  }
  else
  {
    serial.println("Failed to disconnect wifi");
  }
}

#endif

// https://blog.csdn.net/Naisu_kun/article/details/86165403