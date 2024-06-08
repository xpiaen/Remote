#pragma once
#include "Resource.h"
#include <map>
#include "ServerSocket.h"
#include "EdoyunTools.h"
#include <direct.h>
#include <atlimage.h>
#include <stdio.h>
#include <io.h>
#include <list>

#include "LockDialog.h"

class CCommand
{
public:
	CCommand();
	~CCommand(){}
	int ExcuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)();//��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction;//������ŵ����ܵ�ӳ��
    CLockDialog dlg;
    unsigned threadid;
protected:
    static unsigned __stdcall threadLockDlg(void* arg) 
    {
        CCommand* pCommand = (CCommand*)arg;
        pCommand->threadLockDlgMain();
        _endthreadex(0);
        return 0;
    }

    void threadLockDlgMain() {
        TRACE("%s %d %d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
        dlg.Create(IDD_DIALOG_INFO, NULL);
        dlg.ShowWindow(SW_SHOW);
        //�ڱκ�̨����
        CRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
        rect.bottom = LONG(rect.bottom * 1.10);
        TRACE("right:%d bottom:%d\r\n", rect.right, rect.bottom);
        dlg.MoveWindow(rect);
        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText) {
            CRect rcText;
            pText->GetWindowRect(&rcText);
            int rcTextWidth = rcText.Width();//��ȡ�ı����
            int rcTextHeight = rcText.Height();//��ȡ�ı��߶�
            int x = (rect.right - rcTextWidth) / 2;
            int y = (rect.bottom - rcTextHeight) / 2;
            pText->MoveWindow(x, y, rcTextWidth, rcTextHeight);
        }
        //���ô����ö�
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        //������깦��
        ShowCursor(false);//�������
        ::ShowWindow(::FindWindow(_T("Shell_Traywnd"), NULL), SW_HIDE);//����������
        dlg.GetWindowRect(&rect);//��ȡ����λ��
        rect.left = 0;
        rect.top = 0;
        rect.right = 1;
        rect.bottom = 1;
        ClipCursor(rect);//���������Χ
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {//GetMessageֻ�ܱ��߳̽�����Ϣ�����ܿ��߳�
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN) {
                TRACE("msg:%08x wparam:%08x lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x41) {//����a��  �˳�  ESC(1B)
                    break;
                }
            }
        }
        ClipCursor(NULL);//�ָ������Χ
        ShowCursor(true);//��ʾ���
        ::ShowWindow(::FindWindow(_T("Shell_Traywnd"), NULL), SW_SHOW);//��ʾ������
        dlg.DestroyWindow();
    }

    int MakeDriverInfo() {
        std::string result;
        for (int i = 1; i <= 26; i++) {
            if (_chdrive(i) == 0) {
                if (result.size() > 0) {
                    result += ',';
                }
                result += 'A' + i - 1;
            }
        }
        CPacket pack(1, (BYTE*)result.c_str(), result.size());//������Ϣ���ݴ��
        CEdoyunTools::Dump((BYTE*)pack.Data(), pack.Size());
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int MakeDirectoryInfo() {
        std::string strPath;
        //std::list<FILEINFO> listFileInfos;
        if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
            OutputDebugStringA("��ǰ����ǻ�ȡ�ļ��б�����������󣡣���");
            return -1;
        }
        if (_chdir(strPath.c_str()) != 0) {
            FILEINFO fileInfo;
            fileInfo.HasNext = FALSE;
            CPacket pack(2, (BYTE*)&fileInfo, sizeof(fileInfo));
            CServerSocket::getInstance()->Send(pack);
            OutputDebugStringA("û��Ȩ�޷���Ŀ¼������");
            return -2;
        }
        _finddata_t fdata;
        int hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            OutputDebugStringA("û���ҵ��κ��ļ�������");
            FILEINFO fileInfo;
            fileInfo.HasNext = FALSE;
            CPacket pack(2, (BYTE*)&fileInfo, sizeof(fileInfo));
            CServerSocket::getInstance()->Send(pack);
            return -3;
        }
        int Count = 0;
        do {
            FILEINFO fileInfo;
            fileInfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
            memcpy(fileInfo.szFileName, fdata.name, strlen(fdata.name));
            TRACE("fileinfoname:%s\r\n", fileInfo.szFileName);
            CPacket pack(2, (BYTE*)&fileInfo, sizeof(fileInfo));
            CServerSocket::getInstance()->Send(pack);
            Count++;
        } while (!_findnext(hfind, &fdata));
        TRACE("Server ount:%d\r\n", Count);
        //������Ϣ�����ƶ�
        FILEINFO fileInfo;
        fileInfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&fileInfo, sizeof(fileInfo));
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int RunFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//���ļ�
        CPacket pack(3, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int DownloadFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        long long data = 0;
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        if (err != 0) {
            CPacket pack(4, (BYTE*)&data, 8);
            CServerSocket::getInstance()->Send(pack);
            return -1;
        }
        if (pFile != NULL) {
            fseek(pFile, 0, SEEK_END);
            data = _ftelli64(pFile);
            CPacket head(4, (BYTE*)&data, 8);
            CServerSocket::getInstance()->Send(head);
            fseek(pFile, 0, SEEK_SET);
            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);
                CPacket pack(4, (BYTE*)buffer, rlen);
                CServerSocket::getInstance()->Send(pack);
            } while (rlen >= 1024);
            fclose(pFile);
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int DeleteLocalFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        TCHAR sPath[MAX_PATH] = _T("");
        //mbstowcs(sPath, strPath.c_str(), strPath.size());//������������
        MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
        DeleteFileA(strPath.c_str());
        CPacket pack(9, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("Send ret = %d\r\n", ret);
        return 0;
    }

    int MouseEvent() {
        MOUSEEV mouse;
        if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
            DWORD nFlags = 0;
            switch (mouse.nButton) {
            case 0://���
                nFlags = 1;
                break;
            case 1://�Ҽ�
                nFlags = 2;
                break;
            case 2://�м�
                nFlags = 4;
                break;
            case 4://�ް���
                nFlags = 8;
            }
            if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
            switch (mouse.nAction)
            {
            case 0://����
                nFlags |= 0x10;
                break;
            case 1://˫��
                nFlags |= 0x20;
                break;
            case 2://����
                nFlags |= 0x40;
                break;
            case 3://�ſ�
                nFlags |= 0x80;
                break;
            default:
                break;
            }
            TRACE("nFlags:%08X x:%d y:%d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
            switch (nFlags) {
            case 0x21://���˫��
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x11://�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());//GetMessageExtraInfo��ȡ������Ϣ
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x41://�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x81://����ſ�
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x22://�Ҽ�˫��
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x12://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x42://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82://�Ҽ��ſ�
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x24://�м�˫��
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x14://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x44://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84://�м��ſ�
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x80://����������ƶ�
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break;
            }
            CPacket pack(4, NULL, 0);
            CServerSocket::getInstance()->Send(pack);
        }
        else {
            OutputDebugString(_T("��ȡ����������ʧ�ܣ�����"));
            return -1;
        }
        return 0;
    }

    int SendScreen() {
        CImage screen;//GDI
        HDC hSctreen = ::GetDC(NULL);
        int nBitPerPixel = GetDeviceCaps(hSctreen, BITSPIXEL);
        int nWidth = GetDeviceCaps(hSctreen, HORZRES);
        int nHeight = GetDeviceCaps(hSctreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel);
        //BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hSctreen, 0, 0, SRCCOPY);
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        BitBlt(screen.GetDC(), 0, 0, screenWidth, screenHeight, hSctreen, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hSctreen);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMem == NULL)return -1;
        IStream* pStream = NULL;
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (ret == S_OK) {
            screen.Save(pStream, Gdiplus::ImageFormatPNG);
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);
            PBYTE pData = (PBYTE)GlobalLock(hMem);
            size_t nSize = GlobalSize(hMem);
            //TRACE("=======================ScreenSize:%d\r\n", nSize);
            //TRACE("width:%d height:%d\r\n",screenWidth, screenHeight);
            CPacket pack(6, pData, nSize);
            CServerSocket::getInstance()->Send(pack);
            GlobalUnlock(hMem);
        }
        pStream->Release();
        GlobalFree(hMem);
        screen.ReleaseDC();
        return 0;
    }

    int LockMachine() {
        if ((dlg.m_hWnd) == NULL || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
            //_beginthread(threadLockDlg, 0, NULL);
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
            TRACE("threadid:%d\r\n", threadid);
        }
        CPacket pack(7, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int UnlockMachine() {
        //dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
        //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
        PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
        CPacket pack(8, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int TestConnect() {
        CPacket pack(1981, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("Test Send ret = %d\r\n", ret);
        return 0;
    }
};

