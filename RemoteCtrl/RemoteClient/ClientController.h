#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include <map>

#define WM_SEND_PACK (WM_USER + 1)//发送包数据
#define WM_SEND_DATA (WM_USER + 2)//发送数据
#define WM_SHOW_STATUS (WM_USER + 3)//显示状态
#define WM_SHOW_WATCH (WM_USER + 4)//远程监控
#define WM_SEND_MESSAGE (WM_USER + 0x1000)//自定义消息处理

class CClientController
{
public:
	static CClientController* getInstance();//获取全局唯一实例，单例模式
	int InitController();//初始化
	int Invoke(CWnd*& pMainWnd);//启动
	LRESULT SendMessage(MSG msg);//发送消息
protected:
	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg){
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
		m_mapFunc[WM_SEND_PACK] = &CClientController::OnSendPack;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, INFINITE);//等待线程结束,防止线程泄漏
	}
	void threadFunc();
	static unsigned __stdcall threadEntry(void* arg);//线程函数入口
	static void releaseInstance() {
		if (m_instance != nullptr) {   
			delete m_instance;
			m_instance = nullptr;
		}
	}
	LRESULT OnSendPack(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT uMsg, WPARAM wParam, LPARAM lParam);
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
	HANDLE m_hThread;//线程句柄
	unsigned m_nThreadID;//线程ID
	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper() {
			CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

