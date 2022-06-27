#ifndef HPPACKCLIENT_H
#define HPPACKCLIENT_H

#include "HPSocket4C.h"
#include <string>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <vector>
#include <list>
#include <unistd.h>
#include "Logger.h"
#include "PackMessage.hpp"
#include "concurrentqueue.h"

class HPPackClient
{
public:
    HPPackClient(const char *ip, unsigned int port);
    void Start();
    void Stop();
    void SendData(const unsigned char *pBuffer, int iLength);
    virtual ~HPPackClient();
public:
    static moodycamel::ConcurrentQueue<Message::PackMessage> m_PackMessageQueue;
protected:
    static En_HP_HandleResult __stdcall OnConnect(HP_Client pSender, HP_CONNID dwConnID);
    static En_HP_HandleResult __stdcall OnSend(HP_Server pSender, HP_CONNID dwConnID, const BYTE *pData, int iLength);
    static En_HP_HandleResult __stdcall OnReceive(HP_Server pSender, HP_CONNID dwConnID, const BYTE *pData, int iLength);
    static En_HP_HandleResult __stdcall OnClose(HP_Server pSender, HP_CONNID dwConnID, En_HP_SocketOperation enOperation, int iErrorCode);
private:
    std::string m_ServerIP;
    unsigned int m_ServerPort;
    HP_TcpPackClient m_pClient;
    HP_TcpPackClientListener m_pListener;
    static bool m_Connected;
};

#endif // HPPACKCLIENT_H