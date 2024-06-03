// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialogEx)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialogEx)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point)
{//800 450
	//CPoint cur = point;
	CRect clientRect;
	ScreenToClient(&point);//将屏幕坐标转换为客户区坐标
	m_picture.GetWindowRect(&clientRect);//获取远程坐标
	return CPoint((point.x * clientRect.Width() / 800), (point.y * clientRect.Height() / 450));
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 45, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pDlg = (CRemoteClientDlg*)GetParent();
		if (pDlg->isFull()) {
			CRect rect;
			m_picture.GetWindowRect(&rect);//获取图片的大小
			//pDlg->getImage().BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);
			//TRACE("rect.Width()=%d, rect.Height()=%d\n", rect.Width(), rect.Height());
			pDlg->getImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pDlg->getImage().Destroy();
			pDlg->SetImageStatus();
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 2;//双击
	CClientSocket* pSocket = CClientSocket::getInstance();
	CPacket pack = CPacket(5,(BYTE*) & event, sizeof(event));
	pSocket->Send(pack);
	CDialogEx::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	
	CDialogEx::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	//CPoint remote = UserPoint2RemoteScreenPoint(point);
	////封装
	//MOUSEEV event;
	//event.ptXY = remote;
	//event.nButton = 0;//左键
	//event.nAction = 4;//弹起
	//CClientSocket* pSocket = CClientSocket::getInstance();
	//CPacket pack = CPacket(5, (BYTE*)&event, sizeof(event));
	//pSocket->Send(pack);
	CDialogEx::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 2;//右键
	event.nAction = 2;//双击
	CClientSocket* pSocket = CClientSocket::getInstance();
	CPacket pack = CPacket(5, (BYTE*)&event, sizeof(event));
	pSocket->Send(pack);
	CDialogEx::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 2;//右键
	event.nAction = 3;//按下
	CClientSocket* pSocket = CClientSocket::getInstance();
	CPacket pack = CPacket(5, (BYTE*)&event, sizeof(event));
	pSocket->Send(pack);
	CDialogEx::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 2;//右键
	event.nAction = 4;//弹起
	CClientSocket* pSocket = CClientSocket::getInstance();
	CPacket pack = CPacket(5, (BYTE*)&event, sizeof(event));
	pSocket->Send(pack);
	CDialogEx::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;
	event.nAction = 1;//移动
	CClientSocket* pSocket = CClientSocket::getInstance();
	CPacket pack = CPacket(5, (BYTE*)&event, sizeof(event));
	pSocket->Send(pack);
	CDialogEx::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point);
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;//左键
	event.nAction = 3;//按下
	CClientSocket* pSocket = CClientSocket::getInstance();
	CPacket pack = CPacket(5, (BYTE*)&event, sizeof(event));
	pSocket->Send(pack);
}
