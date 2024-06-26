// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//#pragma comment( linker, "/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup")
//#pragma comment( linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup")

//_T("C:\\Users\\Administrator\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe");
#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")

// 唯一的应用程序对象

CWinApp theApp;
using namespace std;

//业务和通用
bool ChooseAutoInvoke(const CString& strPath) {
    if (PathFileExists(strPath)) {
        return true;
    }
    CString strInfo = _T("该程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按下“取消”按钮，退出程序！\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，该程序将只运行一次，不会在系统内留下任何东西！\n");
    LONG ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        if (!CEdoyunTools::WriteRegisterTable(strPath)) {
            MessageBox(NULL, _T("复制文件到运行目录失败！是否权限不足?\r\n程序启动失败!"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
            return false;
        }
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}

enum {
    IocpListEmpty,
    IocpListPush,
    IocpListPop
};

typedef struct IocpParam {
    int nOperator;//操作
    std::string strData;//数据
    _beginthread_proc_type cbFunc;//回调
    IocpParam(int op, const char* data, _beginthread_proc_type func = NULL) {
        nOperator = op;
        strData = data;
        cbFunc = func;
    }
    IocpParam() {
        nOperator = -1;
    }
}IOCP_PARAM;

void threadQueueEntry(HANDLE hIOCP)
{
    std::list<std::string> listString;
    DWORD dwTraansferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* pOverlapped = NULL;
    while (GetQueuedCompletionStatus(hIOCP, &dwTraansferred, &CompletionKey, &pOverlapped, INFINITE)) {
        if ((dwTraansferred == 0) || (CompletionKey == 0)) {
            printf("thread is prepare to exit!\r\n");
            break;
        }
        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
        if (pParam->nOperator == IocpListPush) {
            listString.push_back(pParam->strData);
        }
        else if (pParam->nOperator == IocpListPop) {
            std::string* pStr = NULL;
            if (listString.size() > 0) {
                pStr = new std::string(listString.front());
                listString.pop_front();
            }
            if (pParam->cbFunc) {
                pParam->cbFunc(pStr);
            }
        }
        else if (pParam->nOperator == IocpListEmpty) {
            listString.clear();
        }
        delete pParam;
    }

    _endthread();
}

void func(void* arg) {
    std::string* pstr = (std::string*)arg;
    if (pstr == NULL) {
        printf("list is empty!\r\n");
    }
    else {
        printf("pop from list:%s\r\n", pstr->c_str());
        delete pstr;
    }
}

int main()
{
    if (!CEdoyunTools::Init())return 1;
    
    printf("press any key to exit...\r\n");
    HANDLE hIOCP = INVALID_HANDLE_VALUE;//IO Completion Port
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//创建IOCP    epoll的区别点1
    if (hIOCP == NULL) {
        printf("CreateIoCompletionPort failed with %d\n", GetLastError());
        return 1;
    }
    HANDLE hThread = (HANDLE)_beginthread(threadQueueEntry, 0, hIOCP);//启动IOCP线程

    ULONGLONG tick = GetTickCount64();
    while (_kbhit() != 0) {//完成端口 把请求与实现 分离了
        if (GetTickCount64() - tick > 1300) {
            PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hello world!"), NULL);//通知IOCP线程退出
        }
        if (GetTickCount64() - tick > 2000) {
            PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world!"), NULL);//通知IOCP线程退出
            tick = GetTickCount64();
        }
        Sleep(1);
    }
    if (hIOCP != NULL) {
        PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);//通知IOCP线程退出
        WaitForSingleObject(hThread, INFINITE);//等待IOCP线程退出
    }

    CloseHandle(hIOCP);

    printf("exit done!\r\n");
    ::exit(0);
    /*
    if (CEdoyunTools::IsAdmin()) {
        if (!CEdoyunTools::Init())return 1;
        if (ChooseAutoInvoke(INVOKE_PATH)) {
            CCommand cmd;
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret) {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化网络，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
                break;
            case -2:
                MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                break;
            }
        }
    }
    else {
        if (!CEdoyunTools::RunAdmin()) {
            CEdoyunTools::ShowError();
            return 1;
        }
    }
    */
    return 0;
}
