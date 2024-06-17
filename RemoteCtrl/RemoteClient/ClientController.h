#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "ClientSocket.h"
#include "EdoyunTools.h"
#include <map>


#define WM_SHOW_STATUS (WM_USER + 3)//显示状态
#define WM_SHOW_WATCH (WM_USER + 4)//远程监控
#define WM_SEND_MESSAGE (WM_USER + 0x1000)//自定义消息处理

//业务逻辑和流程，是随时可能发生改变的，所以不建议直接调用，而是通过Controller类来调用。

class CClientController
{
public:
	static CClientController* getInstance();//获取全局唯一实例，单例模式
	int InitController();//初始化
	int Invoke(CWnd*& pMainWnd);//启动
	LRESULT SendMessage(MSG msg);//发送消息
	void UpdateAddress(int nIP, int nPort) {//更新地址
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {//处理命令
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {//关闭套接字
		CClientSocket::getInstance()->CloseSocket();
	}
	//1 查看磁盘分区
	//2 查看指定目录下的文件
	//3 打开文件
	//4 下载文件
	//5 鼠标操作
	//6 发送屏幕内容
	//7 锁机
	//8 解锁
	//9 删除文件
	//1981 测试连接
	//返回值：是命令号，如果小于0，则出错
    // 实现
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
	HANDLE m_ThreadDownLoad;
	HANDLE m_ThreadWatch;
	bool m_isClosed;//监视界面是否关闭
	CString m_strRemote;//下载文件的远程路径
	CString m_strLocal;//下载文件的本地路径
	unsigned m_nThreadID;//线程ID
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

