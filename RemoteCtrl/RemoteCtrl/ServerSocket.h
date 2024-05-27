#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
class CPacket{
public:
	CPacket():sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD ncmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = ncmd;
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
		sSum = 0;
		for (size_t i = 0; i < nSize; i++) {
			sSum += BYTE(strData[i] & 0xFF);
		}
	}
	CPacket(const CPacket& packet) {
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0) {
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;//����ͷ��
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {//�����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ������أ�����ʧ��
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); 
		i += 4;
		if (nLength + i > nSize) {//��δ��ȫ���յ������أ�����ʧ��
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 2 - 2);
			i+=nLength - 2 - 2;
		}
		sSum = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j] & 0xFF);
		}
		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
	}
	~CPacket(){}
	CPacket& operator=(const CPacket& packet) {
		if (this != &packet) {
			sHead = packet.sHead;
			nLength = packet.nLength;
			sCmd = packet.sCmd;
			strData = packet.strData;
			sSum = packet.sSum;
		}
		return *this;;
	}
	int Size() {//�����ݴ�С
		return nLength + 2 + 4;
	}
	const char* Data() {
		strOut.resize(nLength + 2 + 4);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)(pData) = nLength;
		pData += 4;
		*(WORD*)(pData) = sCmd;
		pData += 2;
		if (strData.size() > 0) {
			memcpy(pData, strData.c_str(), strData.size());
			pData += strData.size();
		}
		*(WORD*)(pData) = sSum;
		pData += 2;
		return strOut.c_str();
	}
public:
	WORD sHead;//��ͷ,�̶�λFE FF
	DWORD nLength;//������(�ӿ������ʼ������У�������
	WORD sCmd;//��������
	std::string strData;//������
	WORD sSum;//��У��
	std::string strOut;//������
};
#pragma pack(pop)

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {//��̬����û��thisָ�룬ֻ�����������þ�̬��Ա����
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket() {
		if (m_sock == -1)return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;//htonl�����������ֽ�˳��ת��Ϊ�����ֽ�˳��
		serv_addr.sin_port = htons(9527);
		//��
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)return false;
		//����
		if (listen(m_sock, 1) == -1)return false;

		return true;
	}

	bool AcceptClient()
	{
		sockaddr_in clnt_addr;
		int clnt_len = sizeof(clnt_addr);
		m_client = accept(m_sock, (sockaddr*)&clnt_addr, &clnt_len);//TODO: ����ֵ����
		if (m_client == -1)return false;
		return true;
	}
#define BUFFER_SIZE 4096
	int  DealCommand() {
		if (m_client == -1)return -1;
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, sizeof(buffer));
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE -len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_client == -1)return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_client == -1)return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath) {
		if (m_packet.sCmd == 2) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
private:
	CPacket m_packet;
	SOCKET m_client;
	SOCKET m_sock;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {
		m_client = INVALID_SOCKET;//��Ϊ-1
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,������������!"), _T("��ʼ������!"), MB_OK | MB_ICONERROR);
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
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			// ��ʼ��ʧ�ܣ��������
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