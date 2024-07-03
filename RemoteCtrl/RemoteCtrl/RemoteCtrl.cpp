// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>
#include "CEdoyunQueue.h"
#include "MSWSock.h"
#include "EdoyunServer.h"

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

void iocp();

void udp_server();
void udp_client(bool ishost = true);

int main(int argc,char* argv[])
{
    if (!CEdoyunTools::Init())return 1;

    if (argc == 1) {//服务器
        char wstrDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, wstrDir);
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        memset(&pi, 0, sizeof(pi));
        string strCmd = argv[0];
        strCmd.reserve(strCmd.size() + 2); // 预留足够的空间
        strCmd += " 1";
        BOOL bRet = CreateProcessA(NULL,(LPSTR)strCmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, wstrDir, &si, &pi);
        if (bRet) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            TRACE("进程ID:%d\r\n", pi.dwProcessId);
            TRACE("线程ID:%d\r\n", pi.dwThreadId);
            strCmd += " 2";
            bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, wstrDir, &si, &pi);
            if (bRet) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                TRACE("进程ID:%d\r\n", pi.dwProcessId);
                TRACE("线程ID:%d\r\n", pi.dwThreadId);
                udp_server();
            }
        }
    }
    else if (argc == 2) {//主客户端
        udp_client();
    }
    else {//从客户端
        udp_client(false);
    }

    //iocp();

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

class COverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    char m_buffer[4096];
    COverlapped() {
        m_operator = 0;
        memset(&m_overlapped, 0, sizeof(OVERLAPPED));
        memset(m_buffer, 0, sizeof(m_buffer));
    }
};

void iocp()
{
    EdoyunServer server;
    server.StartService();
    getchar();
}

void udp_server()
{
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    getchar();
}

void udp_client(bool ishost)
{
    if(ishost)
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    else
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
}
