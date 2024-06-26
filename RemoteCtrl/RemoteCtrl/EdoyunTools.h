#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>

class CEdoyunTools
{
public:
    static void Dump(BYTE* pData, size_t nSize) 
    {//����ֽ���
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
    {//���ֽ���ת��ΪͼƬ
        BYTE* pData = (BYTE*)strBuffer.c_str();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//ΪͼƬ���������ڴ�
        if (hMem == NULL) {
            TRACE("�ڴ治�㣡");
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
            if ((HBITMAP)image != NULL)image.Destroy();//�ͷ�ԭ����λͼ
            image.Load(pStream);
        }
        return hRet;
    }
    static bool IsAdmin()
    {//�жϵ�¼�˻��Ƿ��ǹ���ԱȨ��
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            ShowError();
            return false;
        }
        TOKEN_ELEVATION eve;//�����ṹ��,���ڻ�ȡȨ����Ϣ
        DWORD len = 0;
        if (!GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len)) {//��ȡȨ����Ϣ
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
    {//��ȡ����ԱȨ�޲��Թ���ԱȨ���������г���
        //���ز����� ����Admnistrator�˻� ��ֹ������ֻ�ܵ�¼���ؿ���̨
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, _T(""), LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
        if (!ret) {
            ShowError();//TODO:ȥ��������Ϣ
            MessageBox(NULL, sPath, _T("��������ʧ�ܣ�"), 0);//TODO:ȥ��������Ϣ
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
        //strerror(errno);//��׼c���Կ�
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMessageBuf,
            0, NULL);
        OutputDebugString(lpMessageBuf);
        LocalFree(lpMessageBuf);
    }

    //����������ʱ�򣬳����Ȩ���Ǹ����û���
    //�������Ȩ�޲�һ�£���ᵼ�³�������ʧ��
    //���������Ի���������Ӱ�죬�������dll(��̬�⣩�����������ʧ��
    //��������Щdll��system32�������sysWOW64���桿
    //system32��Ϊ64λ����sysWOW64��Ϊ32λ����
    //��ʹ�þ�̬�⣬���Ƕ�̬�⡿
    static bool WriteRegisterTable(const CString& strPath)
    {//ͨ���޸�ע���ʵ�ֿ�������
        CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CopyFile(sPath, strPath, FALSE);
        //fopen CFile system(copy) CopyFile OpenFile
        if (!ret) {
            MessageBox(NULL, _T("�����ļ�ʧ�ܣ��Ƿ�Ȩ�޲���?\r\n��������ʧ��!"), _T("����"), MB_TOPMOST | MB_ICONERROR);
            return false;
        }
        HKEY hKey = NULL;
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲���?\r\n��������ʧ��!"), _T("����"), MB_TOPMOST | MB_ICONERROR);
            return false;
        }
        ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲���?\r\n��������ʧ��!"), _T("����"), MB_TOPMOST | MB_ICONERROR);
            return false;
        }
        RegCloseKey(hKey);
        return true;;
    }
    /**
    * ��bug��˼·
    * 0 �۲�����
    * 1 ��ȷ����Χ
    * 2 �������������
    * 3 ���Ի��ߴ���־����λ����
    * 4 �������
    * 5 ��֤�޸�/��ʱ����֤/�����֤/��������֤
    **/
    static BOOL WriteStartupDir(const CString& strPath)
    {//ͨ�����ļ����Ƶ�����Ŀ¼ʵ�ֿ�������
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
            // TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
            wprintf(L"����: GetCommandLine ʧ��\n");
            return false;
        }
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
            wprintf(L"����: MFC ��ʼ��ʧ��\n");
            return false;
        }
        return true;
    }
};

