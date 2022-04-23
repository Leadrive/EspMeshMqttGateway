#ifndef _SERIAL_H_
#define _SERIAL_H_
#include "common.h"
#include <HardwareSerial.h>

/* 串口波特率 */
#define SERIAL_BAUD_RATE 115200
/* log串口波特率 */
#define SERIAL_LOGGING_BAUD_RATE SERIAL_BAUD_RATE
/* 串口缓冲区大小 */
#define SERIAL_BUFFER_SIZE 2048
/* log也通过串口0输出（只使用串口0） */
#define USE_SERIAL_0_FOR_LOGGING
/* 使用ArduinoJson反序列化串口输入的数据 */
#define USE_ARDUINOJSON_FOR_DESERIALIZATION

#ifdef USE_ARDUINOJSON_FOR_DESERIALIZATION
#include <ArduinoJson.h>
#endif

HardwareSerial serial(0);
#ifdef USE_SERIAL_0_FOR_LOGGING
#define serial_log serial
#else
HardwareSerial serial_log(1); // 调试信息输出
#endif

#ifdef DISABLE_DEBUG_PRINT
#define DebugPrint(...)
#define DebugPrintf(...)
#define DebugPrintln(...)
#else
#define DebugPrint(...) serial_log.print(__VA_ARGS__)
#define DebugPrintf(...) serial_log.printf(__VA_ARGS__)
#define DebugPrintln(...) serial_log.println(__VA_ARGS__)
#endif

#define Print(...) serial.print(__VA_ARGS__)
#define Printf(...) serial.printf(__VA_ARGS__)
#define Println(...) serial.println(__VA_ARGS__)

#ifdef USE_ARDUINOJSON_FOR_DESERIALIZATION
std::function<void(JsonDocument &)> onSerialDataArrival;
#else
std::function<void(char *)> onSerialDataArrival;
#endif

void serialEventRun()
{
    if (serial.available() > 0)
    {
#ifdef USE_ARDUINOJSON_FOR_DESERIALIZATION
        DynamicJsonDocument doc(SERIAL_BUFFER_SIZE * 1.5);
        DeserializationError error = deserializeJson(doc, serial);
        if (error != DeserializationError::Code::Ok)
        {
            Print(F("deserializeJson() failed: "));
            Println(error.f_str());
            return;
        }
        if (onSerialDataArrival != nullptr)
        {
            DebugPrintln("serial: received data");
            onSerialDataArrival(doc);
        }
        else
        {
            DebugPrintln("WARNING: onSerialDataArrival = nullptr");
        }
#else // 自己接收串口原始数据
        char receive_buffer[SERIAL_BUFFER_SIZE];
        char *buffer_p = receive_buffer;
        uint32_t length = serial.readBytesUntil('\n', receive_buffer, SERIAL_BUFFER_SIZE - 1); //需要在最后追加0
        uint32_t i = 0;
        while (i < length) //跳过开头的换行和空格
        {
            if (receive_buffer[i] == '\r' || receive_buffer[i] == '\n' || receive_buffer[i] == ' ')
            {
                i++;
            }
            else
            {
                break;
            }
        }
        if (i == length) //全是空格
        {
            return;
        }
        uint32_t j = length - 1;
        while (j > i) //跳过最后的换行和空格
        {
            if (receive_buffer[j] == '\r' || receive_buffer[j] == '\n' || receive_buffer[j] == ' ')
            {
                j--;
            }
            else
            {
                break;
            }
        }
        length = j - i + 1;
        buffer_p += i; //指针移到开始有数据的地方
        receive_buffer[j + 1] = '\0';
        if (onSerialDataArrival != nullptr)
        {
            DebugPrintf("Serial: received data: '%s';length: %d\r\n", buffer_p, length);
            onSerialDataArrival(buffer_p);
        }
        else
        {
            DebugPrintln("WARNING: onSerialDataArrival = nullptr");
        }
#endif
    }
}

/* template <typename L>
void setSerialCallback(L fun)
{
    onSerialDataArrival = fun;
}*/
void setSerialCallback(std::function<void(JsonDocument &)> cb)
{
    onSerialDataArrival = cb;
}

void serial_init()
{
    serial.setRxBufferSize(SERIAL_BUFFER_SIZE);
#ifndef USE_SERIAL_0_FOR_LOGGING
    serial_log.setRxBufferSize(SERIAL_BUFFER_SIZE);
#endif

#ifdef ESP32
    serial.begin(SERIAL_BAUD_RATE, SERIAL_8N1, 3, 1);
#ifndef USE_SERIAL_0_FOR_LOGGING
    serial_log.begin(SERIAL_LOGGING_BAUD_RATE, SERIAL_8N1, -1, 13);
#endif
#else // ESP8266
    serial.begin(SERIAL_BAUD_RATE, SERIAL_8N1, SERIAL_FULL, 1);
#ifndef USE_SERIAL_0_FOR_LOGGING
    serial_log.begin(SERIAL_LOGGING_BAUD_RATE, SERIAL_8N1, SERIAL_TX_ONLY, 2);
#endif
#endif
}

#endif