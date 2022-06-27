#include "HPPackClient.h"

bool HPPackClient::m_Connected;
moodycamel::ConcurrentQueue<Message::PackMessage> HPPackClient::m_PackMessageQueue(1 << 1024);

extern Utils::Logger *gLogger;

HPPackClient::HPPackClient(const char *ip, unsigned int port)
{
    m_ServerIP = ip;
    m_ServerPort = port;
    // 创建监听器对象
    m_pListener = ::Create_HP_TcpPackClientListener();
    // 创建 Socket 对象
    m_pClient = ::Create_HP_TcpPackClient(m_pListener);
    // 设置 Socket 监听器回调函数
    ::HP_Set_FN_Client_OnConnect(m_pListener, OnConnect);
    ::HP_Set_FN_Client_OnSend(m_pListener, OnSend);
    ::HP_Set_FN_Client_OnReceive(m_pListener, OnReceive);
    ::HP_Set_FN_Client_OnClose(m_pListener, OnClose);

    HP_TcpPackClient_SetMaxPackSize(m_pClient, 0xFFFF);
    HP_TcpPackClient_SetPackHeaderFlag(m_pClient, 0x169);
    ::HP_TcpClient_SetKeepAliveTime(m_pClient, 30*1000);
    m_Connected = false;
}

void HPPackClient::Start()
{
    if(m_Connected)
        return ;
    char errorString[512] = {0};
    if (::HP_Client_Start(m_pClient, (LPCTSTR)m_ServerIP.c_str(), m_ServerPort, false))
    {
        sprintf(errorString, "HPPackClient::Start connected to server[%s:%d]", m_ServerIP.c_str(), m_ServerPort);
        Utils::gLogger->Log->info(errorString);
    }
    else
    {
        sprintf(errorString, "HPPackClient::Start connected to server[%s:%d] failed, error code:%d error massage:%s",
                m_ServerIP.c_str(), m_ServerPort, ::HP_Client_GetLastError(m_pClient), HP_Client_GetLastErrorDesc(m_pClient));
        Utils::gLogger->Log->warn(errorString);
    }
}

void HPPackClient::Stop()
{
    ::HP_Client_Stop(m_pClient);
    //记录客户端(::HP_Client_GetConnectionID(m_pClient));
}

void HPPackClient::SendData(const unsigned char *pBuffer, int iLength)
{
    static std::list<Message::PackMessage> bufferQueue;
    if(!m_Connected)
    {
        Utils::gLogger->Log->warn("HPPackClient::SendData, disconnected to server");
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        memcpy(&message, pBuffer, sizeof(message));
        bufferQueue.push_back(message);
        return;
    }
    for (auto it = bufferQueue.begin(); it != bufferQueue.end(); it++)
    {
        ::HP_Client_Send(m_pClient, reinterpret_cast<const unsigned char *>(&(*it)), sizeof(*it));
    }
    bufferQueue.clear();

    bool ret = ::HP_Client_Send(m_pClient, pBuffer, iLength);
    if(!ret)
    {
        Utils::gLogger->Log->warn("HPPackClient::SendData failed, sys error code:{}, error message:{}",
                                    SYS_GetLastError(), ::HP_Client_GetLastErrorDesc(m_pClient));
    }
}

HPPackClient::~HPPackClient()
{
    // 销毁 Socket 对象
    ::Destroy_HP_TcpPackClient(m_pClient);
    // 销毁监听器对象
    ::Destroy_HP_TcpPackClientListener(m_pListener);
}

En_HP_HandleResult __stdcall HPPackClient::OnConnect(HP_Client pSender, HP_CONNID dwConnID)
{
    TCHAR szAddress[50];
    int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
    USHORT usPort;
    ::HP_Client_GetLocalAddress(pSender, szAddress, &iAddressLen, &usPort);
    m_Connected = true;
    return HR_OK;
}

En_HP_HandleResult __stdcall HPPackClient::OnSend(HP_Server pSender, HP_CONNID dwConnID, const BYTE *pData, int iLength)
{
    return HR_OK;
}

En_HP_HandleResult __stdcall HPPackClient::OnReceive(HP_Server pSender, HP_CONNID dwConnID, const BYTE *pData, int iLength)
{
    Message::PackMessage message;
    memcpy(&message, pData, sizeof(message));
    m_PackMessageQueue.enqueue(message);
    return HR_OK;
}

En_HP_HandleResult __stdcall HPPackClient::OnClose(HP_Server pSender, HP_CONNID dwConnID, En_HP_SocketOperation enOperation, int iErrorCode)
{
    TCHAR szAddress[50];
    int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
    USHORT usPort;
    ::HP_Client_GetLocalAddress(pSender, szAddress, &iAddressLen, &usPort);
    m_Connected = false;

    char errorString[512] = {0};
    sprintf(errorString, "HPPackClient::OnClose connID:%d %s:%d closed, sys error code:%d, error message:%s",
            dwConnID, szAddress, usPort, SYS_GetLastError(), ::HP_Client_GetLastErrorDesc(pSender));
    Utils::gLogger->Log->warn(errorString);
    return HR_OK;
}