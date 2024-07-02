#include "pch.h"
#include "EdoyunServer.h"

template<EdoyunOperator op>
inline AcceptOverlapped<op>::AcceptOverlapped(){
    m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
    m_operator = EAccept;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024);
    m_server = NULL;
}

template<EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() 
{
    INT lLength = 0, rLength = 0;
    if (m_client->GetBufferSize() > 0) {
        sockaddr* plocal = 0, * premote = 0;
        GetAcceptExSockaddrs(*m_client, 0,
            sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            (sockaddr**)&plocal, &lLength, //本地地址
            (sockaddr**)&premote, &rLength  //远程地址
        );
        memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
        memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
        m_server->BindNewSocket(*m_client);
        int ret = WSARecv((SOCKET)*m_client,m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            //TODO:报错
            TRACE("ret = %d WSARecv failed %d\r\n",ret, WSAGetLastError());
        }
        if (!m_server->NewAccept()) {
            return -2;
        }
    }
    return -1;
}

template<EdoyunOperator op>
inline SendOverlapped<op>::SendOverlapped()
{
    m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
    m_operator = ESend;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}
template<EdoyunOperator op>
int SendOverlapped<op>::SendWorker()
{
    //TODO:
    /*
    * 1 Send可能不会立即完成
    * 2 
    */
    return -1;
}

template<EdoyunOperator op>
inline RecvOverlapped<op>::RecvOverlapped()
{
    m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
    m_operator = ERecv;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}
template<EdoyunOperator op>
int RecvOverlapped<op>::RecvWorker()
{
    int ret = m_client->Recv();
    return ret;
}

EdoyunServer::EdoyunServer(const std::string& ip, short port) :m_pool(10) 
{
    m_hIOCP = INVALID_HANDLE_VALUE;
    m_sock = INVALID_SOCKET;
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

EdoyunServer::~EdoyunServer()
{
    closesocket(m_sock);
    std::map<SOCKET,PCLIENT>::iterator it = m_clients.begin();
    for (; it != m_clients.end(); ++it) {
        it->second.reset();
    }
    m_clients.clear();
    CloseHandle(m_hIOCP);
    m_pool.Stop();
    WSACleanup();
}

bool EdoyunServer::StartService()
{
    CreateSocket();
    if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }
    if (listen(m_sock, 3) == SOCKET_ERROR) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }
    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
    if (m_hIOCP == NULL) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        m_hIOCP = INVALID_HANDLE_VALUE;
        return false;
    }
    CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
    m_pool.Invoke();
    m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
    if (!NewAccept()) {
        return false;
    }
    return true;
}

bool EdoyunServer::NewAccept()
{
    PCLIENT pClient(new EdoyunClient());
    pClient->SetOverlapped(pClient);
    m_clients.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
    if (!AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient)) {
        TRACE("AcceptEx failed with error %d\r\n", WSAGetLastError());
        if (WSAGetLastError() != WSA_IO_PENDING) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
    }
    return true;
}

void EdoyunServer::BindNewSocket(SOCKET s)
{
    CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}

void EdoyunServer::CreateSocket()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    int opt = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

int EdoyunServer::threadIocp()
{
    DWORD tranferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;
    if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, (LPOVERLAPPED*)&lpOverlapped, INFINITE)) {
        if (CompletionKey != 0) {
            EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
            TRACE("pOverlapped->m_operator = %d\r\n", pOverlapped->m_operator);
            pOverlapped->m_server = this;
            switch (pOverlapped->m_operator) {
            case EAccept:
            {
                ACCEPTOVERLAPPED* pAccept = (ACCEPTOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pAccept->m_worker);
            }
            break;
            case ERecv:
            {
                RECVOVERLAPPED* pRecv = (RECVOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pRecv->m_worker);
            }
            break;
            case ESend:
            {
                SENDOVERLAPPED* pSend = (SENDOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pSend->m_worker);
            }
            break;
            case EError:
            {
                ERROROVERLAPPED* pError = (ERROROVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pError->m_worker);
            }
            break;
            }
        }
        else {
            return -1;
        }
    }
    return 0;
}

EdoyunClient::EdoyunClient() :m_isbusy(false), m_used(0), m_flags(0), m_recved(0),
    m_overlapped(new ACCEPTOVERLAPPED()),
    m_recvOl(new RECVOVERLAPPED()),
    m_sendOl(new SENDOVERLAPPED()),
    m_vecSend(this, static_cast<SENDCALLBACK>(&EdoyunClient::SendData))
{
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(m_laddr));
    memset(&m_raddr, 0, sizeof(m_raddr));
}

int EdoyunClient::Recv()
{
    int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
    if (ret <= 0)return -1;
    m_used += (size_t)ret;
    CEdoyunTools::Dump((BYTE*)m_buffer.data(), ret);
    return 0;
}

int EdoyunClient::Send(void* buffer, size_t nSize)
{
    std::vector<char>data(nSize);
    memcpy(data.data(), buffer, nSize);
    if (m_vecSend.PushBack(data)) {
        return 0;
    }
    return -1;
}

int EdoyunClient::SendData(std::vector<char>& data)
{
    if (m_vecSend.Size() > 0) {
        int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_recved, m_flags, &m_sendOl->m_overlapped, NULL);
        if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {
            CEdoyunTools::ShowError();
            return -1;
        }
    }
    return 0;
}
