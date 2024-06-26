#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>

class CEdoyunTools
{
public:
    static void Dump(BYTE* pData, size_t nSize) 
    {//输出字节流
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
    static int Bytes2Image(CImage& image, const std::string& strBuffer) 
    {//将字节流转换为图片
        BYTE* pData = (BYTE*)strBuffer.c_str();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//为图片数据申请内存
        if (hMem == NULL) {
            TRACE("内存不足！");
            Sleep(1);
            return -1;
        }
        IStream* pStream = NULL;
        HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (hRet == S_OK) {
            ULONG length = 0;
            pStream->Write(pData, strBuffer.size(), &length);
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);
            if ((HBITMAP)image != NULL)image.Destroy();//释放原来的位图
            image.Load(pStream);
        }
        return hRet;
    }
    static bool IsAdmin()
    {//判断登录账户是否是管理员权限
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            ShowError();
            return false;
        }
        TOKEN_ELEVATION eve;//声明结构体,用于获取权限信息
        DWORD len = 0;
        if (!GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len)) {//获取权限信息
            ShowError();
            return false;
        }
        CloseHandle(hToken);
        if (len == sizeof(eve)) {
            return eve.TokenIsElevated;
        }
        printf("Length of TokenElevation is %d\n", len);
        return false;
    }
    static bool RunAdmin()
    {//获取管理员权限并以管理员权限重新运行程序
        //本地策略组 开启Admnistrator账户 禁止空密码只能登录本地控制台
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, _T(""), LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
        if (!ret) {
            ShowError();//TODO:去除调试信息
            MessageBox(NULL, sPath, _T("创建进程失败！"), 0);//TODO:去除调试信息
            return false;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    static void ShowError()
    {
        LPWSTR lpMessageBuf = NULL;
        //strerror(errno);//标准c语言库
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMessageBuf,
            0, NULL);
        OutputDebugString(lpMessageBuf);
        LocalFree(lpMessageBuf);
    }

    //开机启动的时候，程序的权限是跟随用户的
    //如果两者权限不一致，则会导致程序启动失败
    //开机启动对环境变量有影响，如果依赖dll(动态库），则可能启动失败
    //【复制这些dll到system32下面或者sysWOW64下面】
    //system32多为64位程序，sysWOW64多为32位程序
    //【使用静态库，而非动态库】
    static bool WriteRegisterTable(const CString& strPath)
    {//通过修改注册表实现开机启动
        CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CopyFile(sPath, strPath, FALSE);
        //fopen CFile system(copy) CopyFile OpenFile
        if (!ret) {
            MessageBox(NULL, _T("复制文件失败！是否权限不足?\r\n程序启动失败!"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
            return false;
        }
        HKEY hKey = NULL;
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足?\r\n程序启动失败!"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
            return false;
        }
        ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足?\r\n程序启动失败!"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
            return false;
        }
        RegCloseKey(hKey);
        return true;;
    }
    /**
    * 改bug的思路
    * 0 观察现象
    * 1 先确定范围
    * 2 分析错误可能性
    * 3 调试或者打日志，定位错误
    * 4 处理错误
    * 5 验证修复/长时间验证/多次验证/多条件验证
    **/
    static BOOL WriteStartupDir(const CString& strPath)
    {//通过将文件复制到启动目录实现开机启动
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
       /* CString strCmd = GetCommandLine();
        strCmd.Replace(_T("\""), _T(""));*/
        return CopyFile(sPath, strPath, FALSE);
        //fopen CFile system(copy) CopyFile OpenFile
    }
    static bool Init()
    {
        HMODULE hModule = ::GetModuleHandle(nullptr);
        if (hModule != nullptr) {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: GetCommandLine 失败\n");
            return false;
        }
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            return false;
        }
        return true;
    }
};

