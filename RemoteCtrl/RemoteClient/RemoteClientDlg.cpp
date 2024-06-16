
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_isClosed = true;
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PROT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PROT, &CRemoteClientDlg::OnEnChangeEditProt)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0xC0A8BD8A;//192.168.189.138
	m_nPort = _T("9527");
	CClientController* pCtrl = CClientController::getInstance();
	pCtrl->UpdateAddress(static_cast<int>(m_server_address), atoi((LPCTSTR)m_nPort));
	UpdateData(FALSE);
	m_statusDlg.Create(IDD_DLG_STATUS,this);//创建下载进度对话框
	m_statusDlg.ShowWindow(SW_HIDE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->SendCommandPacket(1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = CClientController::getInstance()->SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
	}
	CClientSocket* pSock = CClientSocket::getInstance();
	std::string drivers = pSock->GetPacket().strData;
	std::string dr;
	m_tree.DeleteAllItems();
	size_t start = 0;
	for (size_t i = 0; i <= drivers.size(); i++) {
		if (i == drivers.size() || drivers[i] == ',') {
			std::string driverName = drivers.substr(start, i - start);
			driverName += ':';
			HTREEITEM hTemp = m_tree.InsertItem(driverName.c_str(), TVI_ROOT, TVI_LAST);
			m_tree.InsertItem("", hTemp, TVI_LAST);
			start = i + 1;
		}
	}
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strPath, strTmp;
	do {
		strTmp = m_tree.GetItemText(hTree);
		strPath = strTmp + "\\" + strPath;
		hTree = m_tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strPath;
}

//删除树节点的子节点
void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_tree.GetChildItem(hTree);
		if(hSub!=NULL)m_tree.DeleteItem(hSub);
	} while (hSub != NULL);
}


void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTreeSelected = m_tree.GetSelectedItem();
	CString strPath = GetPath(hTreeSelected);
	m_List.DeleteAllItems();
	int mCmd = CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	PFILEINFO pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	//CClientSocket* pSock = CClientController::getInstance();
	while (pFileInfo->HasNext) {
		TRACE("FileName:[%s] isdir:%d\r\n", pFileInfo->szFileName, pFileInfo->IsDirectory);
		if (!pFileInfo->IsDirectory) {
			m_List.InsertItem(0, pFileInfo->szFileName);//插入文件名
		}
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack:%d\r\n", CClientSocket::getInstance()->GetPacket());
		if (cmd < 0)break;
		pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	//CClientController::getInstance()->CloseSocket();
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) {
		return;
	}
	if (m_tree.GetChildItem(hTreeSelected) == NULL) {//如果是文件
		return;
	}
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);//取得点击结点的路径
	//TRACE("path:%s\r\n", strPath);
	int mCmd = CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	PFILEINFO pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	//CClientSocket* pSock = CClientController::getInstance();
	int Count = 0;
	while (pFileInfo->HasNext) {
		//TRACE("FileName:[%s] isdir:%d\r\n", pFileInfo->szFileName, pFileInfo->IsDirectory);
		if (pFileInfo->IsDirectory) {
			if (CString(pFileInfo->szFileName) == "." || CString(pFileInfo->szFileName) == "..")
			{
				int cmd = CClientController::getInstance()->DealCommand();
				//TRACE("ack:%d\r\n", CClientController::getInstance()->GetPacket());
				if (cmd < 0)break;
				pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_tree.InsertItem(pFileInfo->szFileName, hTreeSelected, TVI_LAST);
			m_tree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			m_List.InsertItem(0, pFileInfo->szFileName);//插入文件名
		}
		Count++;
		int cmd = CClientController::getInstance()->DealCommand();
		//TRACE("ack:%d\r\n", pSock->GetPacket());
		if (cmd < 0)break;
		pFileInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	//TRACE("Count:%d\r\n", Count);
	//CClientController::getInstance()->CloseSocket();
}


void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse,ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	POSITION pos = m_List.GetFirstSelectedItemPosition();// 检查是否选择了文件
	if (pos == NULL) {
		return;
	}
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);//弹出右键菜单
	}
}



void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_List.GetSelectionMark();//取得选择的文件索引
	CString strFileName = m_List.GetItemText(nListSelected, 0);//取得选择的文件名
	CString strPath = GetPath(m_tree.GetSelectedItem());//取得当前目录路径
	strPath += strFileName;//拼接文件路径
	int ret = CClientController::getInstance()->DownFile(strPath);
	if (ret != 0)
	{
		MessageBox(_T("文件下载失败！！！"));
		TRACE("文件下载失败： ret=%d\r\n", ret);
	}
}


void CRemoteClientDlg::OnDeleteFile()
{
	int nListSelected = m_List.GetSelectionMark();//取得选择的文件索引
	CString strFileName = m_List.GetItemText(nListSelected, 0);//取得选择的文件名
	CString strPath = GetPath(m_tree.GetSelectedItem());//取得当前目录路径
	strPath += strFileName;//拼接文件路径
	int ret = CClientController::getInstance()->SendCommandPacket(9, true, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	if (ret < 0) {
		AfxMessageBox("执行删除文件命令失败！！！");
	}
	LoadFileCurrent();
}


void CRemoteClientDlg::OnRunFile()
{
	int nListSelected = m_List.GetSelectionMark();//取得选择的文件索引
	CString strFileName = m_List.GetItemText(nListSelected, 0);//取得选择的文件名
	CString strPath = GetPath(m_tree.GetSelectedItem());//取得当前目录路径
	strPath += strFileName;//拼接文件路径
	int ret = CClientController::getInstance()->SendCommandPacket(3, true, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	if (ret < 0){
		AfxMessageBox("执行打开文件命令失败！！！");
	}
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();//获取控件值
	CClientController* pCtrl = CClientController::getInstance();
	pCtrl->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}


void CRemoteClientDlg::OnEnChangeEditProt()
{
	UpdateData();//获取控件值
	CClientController* pCtrl = CClientController::getInstance();
	pCtrl->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}
