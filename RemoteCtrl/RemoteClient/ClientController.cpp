#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;//��̬��Ա����
CClientController* CClientController::m_instance = NULL;

CClientController* CClientController::getInstance()
{
	if (m_instance == nullptr) {
		m_instance = new CClientController();
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = 
		{ 
			{WM_SEND_PACK,&CClientController::OnSendPack},
			{WM_SEND_DATA,& CClientController::OnSendData},
			{WM_SHOW_STATUS,&CClientController::OnShowStatus},
			{WM_SHOW_WATCH,&CClientController::OnShowWatch},
			{(UINT) -1,NULL}
		};
		for (int i = 0; MsgFuncs[i].nMsg != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));//����Ϣ���Ӧ�Ĵ���������
		}
	}
	return nullptr;
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

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	CWatchDialog dlg(&m_remoteDlg);
	m_ThreadWatch = (HANDLE)_beginthread(&CClientController::threadEntryForWatchScreen, 0, this);
	dlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_ThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	while (!m_isClosed) {
		if (m_remoteDlg.isFull() == false) {//�������ݵ�����
			int ret = SendCommandPacket(6);
			if (ret == 6) {
				if (GetImage(m_remoteDlg.GetImage()) == 0) {
					m_remoteDlg.SetImageStatus(true);
				}
				else {
					TRACE("��ȡͼƬʧ�� ret = %d��\r\n",ret);
				}
			}
			else {
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
		int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength());
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

LRESULT CClientController::OnSendPack(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pSocket = CClientSocket::getInstance();
	CPacket* pPacket = (CPacket*)wParam;
	return pSocket->Send(*pPacket);
}

LRESULT CClientController::OnSendData(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pSocket = CClientSocket::getInstance();
	char* pData = (char*)wParam;
	return pSocket->Send(pData, (int)lParam);
}

LRESULT CClientController::OnShowStatus(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatch(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}