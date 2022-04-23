#ifndef _MESH_H_
#define _MESH_H_

#include "eeprom_config.h"

#include <FloodingMesh.h>

#define ESP_NOW_ENCRYPTEDCONNECTION_KEY "HaohaoILoveYou!" // This is the key for encrypting transmissions of encrypted connections.
#define ESP_NOW_HASH_KEY "My QQ:763969031"                // This is the secret key used for HMAC during encrypted connection requests.
#define ESP_NOW_MESSAGE_ENCRYPTION_KEY "15E23C4B7EB6E407"

class MeshMessageProcessingTask : public Task
{
private:
    String messagesReceived; //接收到的待处理的消息
    uint32_t messagesSender_NodeId;
    std::function<void(uint32_t sender, String &message)> onMeshReceivedMessageCallback;

public:
    MeshMessageProcessingTask(uint32_t messagesSender_NodeId, String &messagesReceived, std::function<void(uint32_t sender, String &message)> onMeshReceivedMessageCallback)
    {
        this->messagesSender_NodeId = messagesSender_NodeId;
        this->messagesReceived = messagesReceived;
        this->onMeshReceivedMessageCallback = onMeshReceivedMessageCallback;
        setCallback(
            [this]()
            {
                this->onMeshReceivedMessageCallback(this->messagesSender_NodeId, this->messagesReceived);
                delete this; //析构函数会删除任务
            });
        setInterval(TASK_IMMEDIATE);
        setIterations(TASK_ONCE);
    }
};

class Mesh
{

#define MESH_BROADCAST_ID 0xffffffff
#define MESH_GATEWAY_ID 0U

public:
    uint32_t getNodeId();
    void begin();
    void setOnMeshReceivedMessageCallback(std::function<void(uint32_t sender, String &message)> cb);
    void performMaintenance();
    void sendBroadcast(String &message);
    void sendSingle(uint32_t target, String &message);
    static uint32_t encodeNodeId(const uint8_t *hwaddr);
    inline void delay(uint32_t durationMs);
    uint32_t maxMessageLength();
    Mesh(char *perfix, char *password, uint8_t channel);

private:
    /**
        收到新消息时的回调
        @param message 从Mesh接收的消息字符串。 当消息从该节点转发到其他节点时，将传递对此字符串的修改。 但是，转发的消息仍将使用相同的messageID。因此，它不会被发送到已经收到此messageID的节点。如果要向整个网络发送新消息，请用broadcast。
        @param meshInstance 收到消息的FloodingMesh实例.
        @return 如果数据还要继续传播给其他节点，返回True，否则返回false.
    */
    std::function<bool(String &, FloodingMesh &)> meshMessageHandler;
    uint32_t generateNodeId();
    uint32_t nodeId = generateNodeId();
    FloodingMesh *floodingMesh;
    std::function<void(uint32_t sender, String &message)> onMeshReceivedMessageCallback;
};

Mesh::Mesh(char *perfix, char *password, uint8_t channel)
{
    meshMessageHandler = [&](String &message, FloodingMesh &meshInstance) -> bool
    {
        const char *pMsg = message.c_str();
        char *pEnd;
        uint32_t targetNodeId = strtoul(pMsg, &pEnd, 10);
        // DebugPrintf("targetNodeId: %d\r\n", targetNodeId);
        if (targetNodeId != nodeId && targetNodeId != MESH_BROADCAST_ID && targetNodeId != MESH_GATEWAY_ID) //网关id:0,特殊处理
        {
            // DebugPrintln("other's message");
            return true; //其他设备的单播消息，不做处理直接转发
        }
        //自己的单播或广播消息
        String messagesReceived = message.substring(pEnd - pMsg + 1); //去掉开头的发送目标信息
        uint8_t senderMAC[] = {0, 0, 0, 0, 0, 0};
        meshInstance.getEspnowMeshBackend().getSenderMac(senderMAC);
        uint32_t messagesSender_NodeId = encodeNodeId(senderMAC);
        // processingMessageTask.enable();           //在返回后准备处理消息
        Printf("[Mesh ==> Node] NodeId = %u\r\n", messagesSender_NodeId);
        DebugPrintf("\tMsg = %s\r\n", messagesReceived.c_str());
        //创建消息处理任务;
        MeshMessageProcessingTask *task = new MeshMessageProcessingTask(messagesSender_NodeId, messagesReceived, onMeshReceivedMessageCallback);
        runner->addTask(*task);
        task->enable();
        return targetNodeId == MESH_BROADCAST_ID; //如果是广播消息，还要继续转发，返回真
    };
    floodingMesh = new FloodingMesh(meshMessageHandler, password, ESP_NOW_ENCRYPTEDCONNECTION_KEY, ESP_NOW_HASH_KEY, perfix, String(nodeId), false, channel);
}

uint32_t Mesh::generateNodeId()
{
    uint8_t MAC[] = {0, 0, 0, 0, 0, 0};
    WiFi.softAPmacAddress(MAC);
    return encodeNodeId(MAC);
}

inline void Mesh::delay(uint32_t durationMs)
{
    floodingMeshDelay(durationMs);
}

uint32_t Mesh::encodeNodeId(const uint8_t *hwaddr)
{
    uint32_t value = 0;
    value |= hwaddr[2] << 24;
    value |= hwaddr[3] << 16;
    value |= hwaddr[4] << 8;
    value |= hwaddr[5];
    return value;
}

void Mesh::sendSingle(uint32_t target, String &message)
{
    Printf("[Mesh <== Node] NodeId = %u\r\n", target);
    DebugPrintf("\tMsg = %s\r\n", message.c_str());
    char buf[11];
    String messageHead;
    ultoa(target, buf, 10);
    messageHead.concat(buf);
    messageHead.concat('|');
    messageHead.concat(message);
    floodingMesh->broadcast(messageHead); //禁止在接收回调中调用此函数（官方文档里说的），否则崩溃！
}

void Mesh::sendBroadcast(String &message)
{
    sendSingle(MESH_BROADCAST_ID, message);
}

void Mesh::setOnMeshReceivedMessageCallback(std::function<void(uint32_t sender, String &message)> cb)
{
    onMeshReceivedMessageCallback = cb;
}
uint32_t Mesh::getNodeId()
{
    return nodeId;
}

inline void Mesh::performMaintenance()
{
    // floodingMesh.performMeshMaintenance();
    floodingMesh->performMeshInstanceMaintenance();
}

uint32_t Mesh::maxMessageLength()
{
    uint32_t l_en = floodingMesh->maxEncryptedMessageLength();
    uint32_t l_uen = floodingMesh->maxUnencryptedMessageLength();
    return l_en < l_uen ? l_en - 12 : l_uen - 12; //减去头信息的12字节
}

void Mesh::begin()
{
    floodingMesh->begin();
    floodingMesh->activateAP();                                                                         // Required to receive messages
    floodingMesh->getEspnowMeshBackend().setEspnowMessageEncryptionKey(ESP_NOW_MESSAGE_ENCRYPTION_KEY); // The message encryption key should always be set manually. Otherwise a default key (all zeroes) is used.
    floodingMesh->getEspnowMeshBackend().setUseEncryptedMessages(true);
}

Mesh *mesh;

void mesh_init()
{
    WiFi.persistent(false);
    mesh->begin();
}

#endif