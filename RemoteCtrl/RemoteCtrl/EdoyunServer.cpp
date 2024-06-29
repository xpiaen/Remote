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
    if (*(LPDWORD)*m_client.get() > 0) {
        GetAcceptExSockaddrs(*m_client, 0,
            sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            (sockaddr**)m_client->GetLocalAddr(), &lLength, //本地地址
            (sockaddr**)m_client->GetRemoteAddr(), &rLength  //远程地址
        );
        int ret = WSARecv((SOCKET)*m_client,m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), *m_client, NULL);
        if (ret == SOCKET_ERROR && WSAGetLastError() == WSA_IO_PENDING) {
            //TODO:报错
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

int EdoyunServer::threadIocp()
{
    DWORD tranferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;
    if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, (LPOVERLAPPED*)&lpOverlapped, INFINITE)) {
        if (tranferred > 0 && (CompletionKey != 0)) {
            EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
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
