﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"


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
        return false;
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

int main()
{
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
    return 0;
}
