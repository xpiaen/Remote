#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;//��̬��Ա����
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == nullptr) {
		m_instance = new CClientController();
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = 
		{ 
			{WM_SHOW_STATUS,&CClientController::OnShowStatus},
			{WM_SHOW_WATCH,&CClientController::OnShowWatch},
			{(UINT) -1,NULL}
		};
		for (int i = 0; MsgFuncs[i].nMsg != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));//����Ϣ���Ӧ�Ĵ�������
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);//�����߳�
	m_statusDlg.Create(IDD_DLG_STATUS,&m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL) {
		return -2;
	}
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,(WPARAM)&info,(LPARAM)&hEvent);
	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);
	return info.result;
}

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClosed, BYTE* pData, int nLength, WPARAM wParam)
{
	TRACE("hWnd: %d cmd��%d %s start %lld\r\n",hWnd, nCmd,__FUNCTION__,GetTickCount64());
	CClientSocket* pSocket = CClientSocket::getInstance();
	bool ret = pSocket->SendPacket(hWnd,CPacket(nCmd, pData, nLength),bAutoClosed,wParam);
	return ret;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();//���صȴ����
	m_remoteDlg.MessageBox(_T("�ļ��������"), _T("���"));
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL, strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox("����û��Ȩ�ޱ�����ļ��������ļ��޷�����������");
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		/*m_ThreadDownLoad = (HANDLE)_beginthread(&CClientController::threadEntryForDownFile, 0, this);
		if (WaitForSingleObject(m_ThreadDownLoad, 0) != WAIT_TIMEOUT) {
			return -1;
		}*/
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);
	m_ThreadWatch = (HANDLE)_beginthread(&CClientController::threadEntryForWatchScreen, 0, this);
	m_watchDlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_ThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {//�������ݵ�����
			std::string strData;
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6,true,NULL,0);
			//TODO:�����Ϣ��Ӧ����WM_SEND_PACK_ACK
			//TODO:���Ʒ���Ƶ��
			if (ret == 6) {
				if (CEdoyunTools::Bytes2Image(m_watchDlg.getImage(),strData) == 0) {
					TRACE("��ȡͼƬ�ɹ���\r\n");
					m_watchDlg.SetImageStatus(true);
				}
				else {
					TRACE("��ȡͼƬʧ�� ret = %d��\r\n",ret);
				}
			}
			else {
				TRACE("��ȡͼƬʧ�� ret = %d��\r\n", ret);
				Sleep(1);
			}
		}
		else Sleep(1);
	}
}

void CClientController::threadEntryForWatchScreen(void* arg)
{
	CClientController* pThis = CClientController::getInstance();
	pThis->threadWatchScreen();
	_endthread();
}

void CClientController::threadDownloadFile()
{
	FILE* pFile = fopen(m_strLocal, "wb+");
	if (pFile == NULL) {
		AfxMessageBox("����û��Ȩ�ޱ�����ļ��������ļ��޷�����������");
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();//���صȴ����
		return;
	}
	CClientSocket* pSock = CClientSocket::getInstance();
	do {
		int ret = SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		if (ret < 0) {
			AfxMessageBox("ִ����������ʧ�ܣ�����");
			TRACE("ִ������ʧ�ܣ� ret:%d\r\n", ret);
			break;
		}
		long long nlength = *(long long*)pSock->GetPacket().strData.c_str();
		if (nlength == 0) {
			AfxMessageBox("�ļ�����Ϊ������޷���ȡ�ļ�������");
			break;
		}
		long long nCount = 0;
		while (nCount < nlength) {
			int ret = pSock->DealCommand();
			if (ret < 0) {
				AfxMessageBox("����ʧ�ܣ�����");
				TRACE("����ʧ�ܣ� ret:%d\r\n", ret);
				break;
			}
			fwrite(pSock->GetPacket().strData.c_str(), 1, pSock->GetPacket().strData.size(), pFile);
			nCount += pSock->GetPacket().strData.size();
		}
	} while (false);
	fclose(pFile);
	pSock->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();//���صȴ����
	m_remoteDlg.MessageBox(_T("�ļ��������"), _T("���"));
}

void CClientController::threadEntryForDownFile(void* arg)
{
	CClientController* pThis = CClientController::getInstance();
	pThis->threadDownloadFile();
	_endthread();
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE* hEvent = (HANDLE*)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* pThis = (CClientController*)arg;
	_endthreadex(0);
	return 0;
}


LRESULT CClientController::OnShowStatus(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatch(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
