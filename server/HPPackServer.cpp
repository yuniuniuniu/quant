#include "HPPackServer.h"

extern Utils::Logger *gLogger;

std::unordered_map<HP_CONNID, Connection> HPPackServer::m_sConnections;
std::unordered_map<HP_CONNID, Connection> HPPackServer::m_newConnections;
Utils::RingBuffer<Message::PackMessage> HPPackServer::m_PackMessageQueue(1 << 10);

HPPackServer::HPPackServer(const char *ip, unsigned int port)
{
    m_ServerIP = ip;
    m_ServerPort = port;
    // 创建监听器对象
    m_pListener = ::Create_HP_TcpPackServerListener();
    // 创建 Socket 对象
    m_pServer = ::Create_HP_TcpPackServer(m_pListener);

    // 设置 Socket 监听器回调函数
    ::HP_Set_FN_Server_OnAccept(m_pListener, OnAccept);
    ::HP_Set_FN_Server_OnSend(m_pListener, OnSend);
    ::HP_Set_FN_Server_OnReceive(m_pListener, OnReceive);
    ::HP_Set_FN_Server_OnClose(m_pListener, OnClose);
    ::HP_Set_FN_Server_OnShutdown(m_pListener, OnShutdown);

    // 设置包头标识与最大包长限制
    ::HP_TcpPackServer_SetMaxPackSize(m_pServer, 0xFFFF);
    ::HP_TcpPackServer_SetPackHeaderFlag(m_pServer, 0x169);
    ::HP_TcpServer_SetKeepAliveTime(m_pServer, 30 * 1000);
}

HPPackServer::~HPPackServer()
{
    // 销毁 Socket 对象
    ::Destroy_HP_TcpPackServer(m_pServer);
    // 销毁监听器对象
    ::Destroy_HP_TcpPackServerListener(m_pListener);
}

void HPPackServer::Start()
{
    char errorString[512] = {0};
    if (::HP_Server_Start(m_pServer, m_ServerIP.c_str(), m_ServerPort))
    {
        sprintf(errorString, "HPPackServer::Start listen to %s:%d successed", m_ServerIP.c_str(), m_ServerPort);
        Utils::gLogger->Log->info(errorString);
    }
    else
    {
        sprintf(errorString, "HPPackServer::Start listen to %s:%d failed, error code:%d error massage:%s",
                m_ServerIP.c_str(), m_ServerPort, ::HP_Client_GetLastError(m_pServer), HP_Client_GetLastErrorDesc(m_pServer));
        Utils::gLogger->Log->warn(errorString);
    }
}

void HPPackServer::Stop()
{
    //停止服务器
    ::HP_Server_Stop(m_pServer);
}

void HPPackServer::SendData(HP_CONNID dwConnID, const unsigned char *pBuffer, int iLength)
{
    bool ret = ::HP_Server_Send(m_pServer, dwConnID, pBuffer, iLength);
    if(!ret)
    {
        char errorString[512] = {0};
        sprintf(errorString, "HPPackServer::SendData failed, sys error:%s, error code:%d, error message:%s",
                SYS_GetLastErrorStr(), HP_Client_GetLastError(m_pServer), HP_Client_GetLastErrorDesc(m_pServer));
        Utils::gLogger->Log->warn(errorString);
    }
}

En_HP_HandleResult __stdcall HPPackServer::OnAccept(HP_Server pSender, HP_CONNID dwConnID, UINT_PTR soClient)
{
    TCHAR szAddress[50];
    int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
    USHORT usPort;
    ::HP_Server_GetRemoteAddress(pSender, dwConnID, szAddress, &iAddressLen, &usPort);
    std::mutex mtx;
    mtx.lock();
    Connection connection;
    memset(&connection, 0, sizeof(connection));
    connection.dwConnID = dwConnID;
    connection.pSender = pSender;
    auto it = m_sConnections.find(dwConnID);
    if (m_sConnections.end() == it)
    {
        m_sConnections.insert(std::pair<HP_CONNID, Connection>(dwConnID, connection));
    }
    mtx.unlock();
    char errorString[512] = {0};
    sprintf(errorString, "HPPackServer::OnAccept accept an new connection dwConnID:%d from %s:%d",  dwConnID, szAddress, usPort);
    Utils::gLogger->Log->info(errorString);
    return HR_OK;
}

En_HP_HandleResult __stdcall HPPackServer::OnSend(HP_Server pSender, HP_CONNID dwConnID, const BYTE *pData, int iLength)
{
    return HR_OK;
}

En_HP_HandleResult __stdcall HPPackServer::OnReceive(HP_Server pSender, HP_CONNID dwConnID, const BYTE *pData, int iLength)
{
    TCHAR szAddress[50];
    int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
    USHORT usPort;
    ::HP_Server_GetRemoteAddress(pSender, dwConnID, szAddress, &iAddressLen, &usPort);
    Message::PackMessage message;
    memcpy(&message, pData, iLength);
    char messageType[32] = {0};
    sprintf(messageType, "0X%X", message.MessageType);
    Utils::gLogger->Log->debug("HPPackServer::OnReceive receive PackMessage, MessageType:{}", messageType);
    // LoginRequest
    if (Message::EMessageType::ELoginRequest == message.MessageType)
    {
        std::mutex mtx;
        mtx.lock();
        auto it = m_sConnections.find(dwConnID);
        if (it != m_sConnections.end())
        {
            it->second.ClientType = message.LoginRequest.ClientType;
            strncpy(it->second.Colo, message.LoginRequest.Colo, sizeof(it->second.Colo));
            strncpy(it->second.Account, message.LoginRequest.Account, sizeof(it->second.Account));
            strncpy(it->second.PassWord, message.LoginRequest.PassWord, sizeof(it->second.PassWord));
            strncpy(it->second.UUID, message.LoginRequest.UUID, sizeof(it->second.UUID));
            char errorString[512] = {0};
            sprintf(errorString, "HPPackServer::OnReceive accept an new Client login from %s:%d, Account:%s PassWord:%s",
                    szAddress, usPort, message.LoginRequest.Account, message.LoginRequest.PassWord);
            Utils::gLogger->Log->info(errorString);
        }
        mtx.unlock();
    }
    m_PackMessageQueue.push(message);
    return HR_OK;
}

En_HP_HandleResult __stdcall HPPackServer::OnClose(HP_Server pSender, HP_CONNID dwConnID, En_HP_SocketOperation enOperation, int iErrorCode)
{
    TCHAR szAddress[50];
    int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
    USHORT usPort;
    ::HP_Server_GetRemoteAddress(pSender, dwConnID, szAddress, &iAddressLen, &usPort);

    std::mutex mtx;
    mtx.lock();
    auto it = m_sConnections.find(dwConnID);
    if (it != m_sConnections.end())
    {
        m_sConnections.erase(dwConnID);
    }
    auto it1 = m_newConnections.find(dwConnID);
    if (it1 != m_newConnections.end())
    {
        m_newConnections.erase(dwConnID);
    }
    mtx.unlock();
    char errorString[512] = {0};
    sprintf(errorString, "HPPackServer::OnClose have an connection dwConnID:%d from %s:%d closed",  dwConnID, szAddress, usPort);
    Utils::gLogger->Log->warn(errorString);

    return HR_OK;
}

En_HP_HandleResult __stdcall HPPackServer::OnShutdown(HP_Server pSender)
{
    return HR_OK;
}