#ifndef HPPACKSERVER_H
#define HPPACKSERVER_H

#include "HPSocket4C.h"
#include <string>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <unistd.h>
#include "Logger.h"
#include "concurrentqueue.h"
#include "PackMessage.hpp"

struct Connection
{
    HP_Server pSender;
    HP_CONNID dwConnID;
    int ClientType;
    char Account[16];
    char PassWord[16];
    char AppName[32];
    char Plugins[256];
    char UUID[32];
};

class HPPackServer
{
public:
    HPPackServer(const char *ip, unsigned int port);
    virtual ~HPPackServer();
    void Start();
    void Stop();
    void SendData(HP_CONNID dwConnID, const unsigned char *pBuffer, int iLength);
protected:
    static En_HP_HandleResult __stdcall OnPrepareListen(HP_Server pSender, UINT_PTR soListen);
    static En_HP_HandleResult __stdcall OnAccept(HP_Server pSender, HP_CONNID dwConnID, UINT_PTR soClient);
    static En_HP_HandleResult __stdcall OnSend(HP_Server pSender, HP_CONNID dwConnID, const BYTE *pData, int iLength);
    static En_HP_HandleResult __stdcall OnReceive(HP_Server pSender, HP_CONNID dwConnID, const BYTE *pData, int iLength);
    static En_HP_HandleResult __stdcall OnClose(HP_Server pSender, HP_CONNID dwConnID, En_HP_SocketOperation enOperation, int iErrorCode);
    static En_HP_HandleResult __stdcall OnShutdown(HP_Server pSender);
public:
    static moodycamel::ConcurrentQueue<Message::PackMessage> m_PackMessageQueue;
    static std::unordered_map<HP_CONNID, Connection> m_sConnections;
    static std::unordered_map<HP_CONNID, Connection> m_newConnections;
private:
    std::string m_ServerIP;
    unsigned int m_ServerPort;
    HP_TcpServer m_pServer;
    HP_TcpServerListener m_pListener;
};

#endif // HPPACKSERVER_H