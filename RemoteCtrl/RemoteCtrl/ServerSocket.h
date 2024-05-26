#pragma once
#include "pch.h"
#include "framework.h"
class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {//静态函数没有this指针，只能用类名调用静态成员函数
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket() {
		if(m_sock == -1)return false;
		//TODO: 校验
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;//htonl（）将主机字节顺序转换为网络字节顺序
		serv_addr.sin_port = htons(9527);
		//绑定
		if(bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr))==-1)return false;
		//监听
		if(listen(m_sock, 1)==-1)return false;
		
		return true;
	}

	bool AcceptClient()
	{
		sockaddr_in clnt_addr;
		int clnt_len = sizeof(clnt_addr);
		m_client = accept(m_sock, (sockaddr*)&clnt_addr, &clnt_len);//TODO: 返回值处理
		if(m_client == -1)return false;
		return true;
	}

	int  DealCommand() {
		if (m_client == -1)return false;
		char buffer[1024] = "";
		while (true) {
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0)return -1;
			//TODO: 命令处理
		}
	}

	bool Send(const char* pData, size_t nSize) {
		return send(m_client, pData, nSize, 0) > 0;
	}
private:
	SOCKET m_client;
	SOCKET m_sock;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {
		m_client = INVALID_SOCKET;//即为-1
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置!"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSAData data;
		if (WSAStartup(MAKELONG(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance() {
		if(m_instance!= NULL){
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};