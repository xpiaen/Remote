// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//#pragma comment( linker, "/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup")
//#pragma comment( linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup")


// 唯一的应用程序对象
// sub_fuc分支
CWinApp theApp;

using namespace std;


void Dump(BYTE* pData, size_t nSize) {
    std::string strOut;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0))strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += '\n';
    OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() {
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if(result.size() > 0) {
                result += ',';
            }
            result += 'A' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());//磁盘信息数据打包
    Dump((BYTE*)pack.Data(), pack.Size());
    //CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <stdio.h>
#include <io.h>
#include <list>
typedef struct file_info{
    file_info() {
        IsInvalid = FALSE;
        IsDirectory = 0;
        HasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }
    BOOL IsInvalid;//是否有效
    BOOL IsDirectory;//是否为目录 0 否 1 是
    BOOL HasNext;//是否有后续 0 否 1 是
    char szFileName[256];//文件名
}FILEINFO,*PFILEINFO;
int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> listFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugStringA("当前命令不是获取文件列表，命令解析错误！！！");
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO fileInfo;
        fileInfo.IsInvalid = TRUE;
        fileInfo.IsDirectory = TRUE;
        fileInfo.HasNext = FALSE;
        memcpy(fileInfo.szFileName, strPath.c_str(), strPath.size());
        //listFileInfos.push_back(fileInfo);
        CPacket pack(2, (BYTE*) & fileInfo, sizeof(fileInfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugStringA("没有权限访问目录！！！");
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugStringA("没有找到任何文件！！！");
        return -3;
    }
    do {
        FILEINFO fileInfo;
        fileInfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
        memcpy(fileInfo.szFileName,fdata.name,strlen(fdata.name));
        //listFileInfos.push_back(fileInfo);
        CPacket pack(2, (BYTE*)&fileInfo, sizeof(fileInfo));
        CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));
    //发送信息到控制端
    FILEINFO fileInfo;
    fileInfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&fileInfo, sizeof(fileInfo));
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误对话框。
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {//1 进度可控性 2 方便对接  3 可行性评估，提早暴露风险
            // TODO: socket bind listen accept read write close
            //套接字初始化
            //server;
           /* CServerSocket* pserver = CServerSocket::getInstance();
            if (pserver->InitSocket() == false) {
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化网络，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            int count = 0;
            while (CServerSocket::getInstance() != NULL) {

                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    count++;
                }
                int ret = pserver->DealCommand();
            }*/
            int nCmd = 1;
            switch(nCmd) {
                case 1://查看磁盘分区
                    MakeDriverInfo();
                    break;
                case 2://查看指定目录下的文件
                    MakeDirectoryInfo();
                    break;
                default:
                    break;
            }
            
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
