#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "ClientSocket.h"
#include "EdoyunTools.h"
#include <map>


#define WM_SHOW_STATUS (WM_USER + 3)//��ʾ״̬
#define WM_SHOW_WATCH (WM_USER + 4)//Զ�̼��
#define WM_SEND_MESSAGE (WM_USER + 0x1000)//�Զ�����Ϣ����

//ҵ���߼������̣�����ʱ���ܷ����ı�ģ����Բ�����ֱ�ӵ��ã�����ͨ��Controller�������á�

class CClientController
{
public:
	static CClientController* getInstance();//��ȡȫ��Ψһʵ��������ģʽ
	int InitController();//��ʼ��
	int Invoke(CWnd*& pMainWnd);//����
	LRESULT SendMessage(MSG msg);//������Ϣ
	void UpdateAddress(int nIP, int nPort) {//���µ�ַ
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {//��������
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {//�ر��׽���
		CClientSocket::getInstance()->CloseSocket();
	}
	//1 �鿴���̷���
	//2 �鿴ָ��Ŀ¼�µ��ļ�
	//3 ���ļ�
	//4 �����ļ�
	//5 ������
	//6 ������Ļ����
	//7 ����
	//8 ����
	//9 ɾ���ļ�
	//1981 ��������
	//����ֵ��������ţ����С��0�������
    // ʵ��
	int SendCommandPacket(int nCmd, bool bAutoClosed = true, BYTE* pData = NULL, int nLength = 0, std::list<CPacket>* plistPacks = NULL);
	int GetImage(CImage& img) {
		CClientSocket* pSock = CClientSocket::getInstance();
		return CEdoyunTools::Bytes2Image(img, pSock->GetPacket().strData);
	}
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	void threadWatchScreen();
	static void threadEntryForWatchScreen(void* arg);
	void threadDownloadFile();
	static void threadEntryForDownFile(void* arg);
	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg){
		m_hThread = INVALID_HANDLE_VALUE;
		m_ThreadDownLoad = INVALID_HANDLE_VALUE;
		m_ThreadWatch = INVALID_HANDLE_VALUE;
		m_isClosed = true;
		m_nThreadID = -1;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, INFINITE);//�ȴ��߳̽���,��ֹ�߳�й©
	}
	void threadFunc();
	static unsigned __stdcall threadEntry(void* arg);//�̺߳������
	static void releaseInstance() {
		if (m_instance != nullptr) {   
			delete m_instance;
			m_instance = nullptr;
		}
	}
	LRESULT OnShowStatus(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatch(UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo{
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			memcpy(&msg, &m, sizeof(MSG));
			result = 0;
		}
		MsgInfo(const MsgInfo& m) {
			memcpy(&msg, &m.msg, sizeof(MSG));
			result = m.result;
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				memcpy(&msg, &m.msg, sizeof(MSG));
				result = m.result;
			}
			return *this;
		}
	}MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	CWatchDialog m_watchDlg;
	CStatusDlg m_statusDlg;
	CRemoteClientDlg m_remoteDlg;
	HANDLE m_hThread;//�߳̾��
	HANDLE m_ThreadDownLoad;
	HANDLE m_ThreadWatch;
	bool m_isClosed;//���ӽ����Ƿ�ر�
	CString m_strRemote;//�����ļ���Զ��·��
	CString m_strLocal;//�����ļ��ı���·��
	unsigned m_nThreadID;//�߳�ID
	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper() {
			//CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

