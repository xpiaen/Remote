#pragma once
#include "EdoyunThread.h"
#include "CEdoyunQueue.h"
#include "EdoyunTools.h"
#include <MSWSock.h>
#include <map>
#include <vector>

enum EdoyunOperator {
    ENone,
    EAccept,
    ERecv,
    ESend,
    EError
};

class EdoyunServer;
class EdoyunClient;
typedef std::shared_ptr<EdoyunClient> PCLIENT;
template<EdoyunOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
template<EdoyunOperator>class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;
template<EdoyunOperator>class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

class EdoyunOverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;//���� �μ�EdoyunOperator
    std::vector<char> m_buffer;//������
    ThreadWorker m_worker;//������
    EdoyunServer* m_server;//����������
    EdoyunClient* m_client;//��Ӧ�Ŀͻ��˶���
    WSABUF m_wsabuffer;
    virtual ~EdoyunOverlapped() {
        m_buffer.clear();
    }
};

template<EdoyunOperator>
class AcceptOverlapped : public EdoyunOverlapped,ThreadFuncBase {
public:
    AcceptOverlapped();
    int AcceptWorker();
};

template<EdoyunOperator>
class RecvOverlapped : public EdoyunOverlapped, ThreadFuncBase {
public:
    RecvOverlapped();
    int RecvWorker();

};

template<EdoyunOperator>
class SendOverlapped : public EdoyunOverlapped, ThreadFuncBase {
public:
    SendOverlapped();
    int SendWorker();
};


template<EdoyunOperator>
class ErrorOverlapped : public EdoyunOverlapped, ThreadFuncBase {
public:
    ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
    }
    int ErrorWorker() {
        //TODO:
        return -1;
    }
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class EdoyunClient : public ThreadFuncBase {
public:
    EdoyunClient();
    ~EdoyunClient() {
        m_buffer.clear();
        closesocket(m_sock);
        m_recvOl.reset();
        m_sendOl.reset();
        m_overlapped.reset();
        m_vecSend.Clear();
    }
    void SetOverlapped(PCLIENT& ptr) {
        m_overlapped->m_client = ptr.get();
        m_recvOl->m_client = ptr.get();
        m_sendOl->m_client = ptr.get();
    }
    operator SOCKET() {
        return m_sock;
    }
    operator PVOID() {
        return &m_buffer[0];
    }
    operator LPOVERLAPPED() {
        return &m_overlapped->m_overlapped;
    }
    operator LPDWORD() {
        return &m_recved;
    }
    LPWSABUF RecvWSABuffer() {
        return &m_recvOl->m_wsabuffer;
    }
    LPWSAOVERLAPPED RecvOverlapped() {
        return &m_recvOl->m_overlapped;
    }
    LPWSABUF SendWSABuffer() {
        return &m_sendOl->m_wsabuffer;
    }
    LPWSAOVERLAPPED SendOverlapped() {
        return &m_sendOl->m_overlapped;
    }
    DWORD& flags() {
        return m_flags;
    }
    sockaddr_in* GetLocalAddr() {
        return &m_laddr;
    }
    sockaddr_in* GetRemoteAddr() {
        return &m_raddr;
    }
    size_t GetBufferSize() const {
        return m_buffer.size();
    }
    int Recv();
    int Send(void* buffer, size_t nSize);
    int SendData(std::vector<char>& data);
private:
    SOCKET m_sock;
    DWORD m_recved;
    DWORD m_flags;
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::shared_ptr<RECVOVERLAPPED> m_recvOl;
    std::shared_ptr<SENDOVERLAPPED> m_sendOl;
    std::vector<char> m_buffer;
    size_t m_used;//��ʹ�õĻ�������С
    sockaddr_in m_laddr;
    sockaddr_in m_raddr;
    bool m_isbusy;
    EdoyunSendQueue<std::vector<char>> m_vecSend;//�������ݶ���
};


class EdoyunServer :
    public ThreadFuncBase
{
public:
    EdoyunServer(const std::string& ip = "0.0.0.0", short port = 9527);
    ~EdoyunServer();
    bool StartService();
    bool NewAccept();
    void BindNewSocket(SOCKET s);
private:
    void CreateSocket();
    int threadIocp();
private:
    EdoyunThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, std::shared_ptr<EdoyunClient>> m_clients;
};

