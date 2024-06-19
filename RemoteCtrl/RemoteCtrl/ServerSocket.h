#pragma once

#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include <list>

typedef void(*SOCKET_CALLBACK)(void*, int, std::list<CPacket>&, CPacket&);

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {//静态函数没有this指针，只能用类名调用静态成员函数
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527) {
		//1 进度可控性 2 方便对接  3 可行性评估，提早暴露风险
		// TODO: socket bind listen accept read write close
		//套接字初始化
		//server;
		bool ret = InitSocket(port);
		if (ret == false)return -1;
		std::list<CPacket> listPackets;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true) {
			if (AcceptClient() == false) {
				if (count >= 3) {
					return -2;
				}
				count++;
			}
			int ret = DealCommand();
			if (ret > 0) {
				m_callback(m_arg, ret, listPackets, m_packet);
				while (listPackets.size() > 0) {
					Send(listPackets.front());
					listPackets.pop_front();
				}
			}
			CloseClient();
		}
		return 0;
	}
protected:
	bool InitSocket(short port) {
		if (m_sock == -1)return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;//htonl（）将主机字节顺序转换为网络字节顺序
		serv_addr.sin_port = htons(port);
		//绑定
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)return false;
		//监听
		if (listen(m_sock, 1) == -1)return false;

		return true;
	}
	bool AcceptClient()
	{
		TRACE("enter AcctptClient\r\n");
		sockaddr_in clnt_addr;
		int clnt_len = sizeof(clnt_addr);
		m_client = accept(m_sock, (sockaddr*)&clnt_addr, &clnt_len);//TODO: 返回值处理3
		//TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1)return false;
		return true;
	}
#define BUFFER_SIZE 4096
	int  DealCommand() {
		if (m_client == -1)return -1;
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("内存不足！\r\n");
			return -2;
		}
		memset(buffer, 0, sizeof(buffer));
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			//TRACE("recv len = %d\r\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE -len);
				index -= len;
				delete[] buffer;
				return m_packet.sCmd;
			}
		}
		delete[] buffer;
		return -1;
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_client == -1)return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_client == -1)return false;
		//Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}
private:
	SOCKET_CALLBACK m_callback;//回调函数
	void* m_arg;
	CPacket m_packet;
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
	bool InitSockEnv() {
		WSAData data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			// 初始化失败，处理错误
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance() {
		if (m_instance != NULL) {
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