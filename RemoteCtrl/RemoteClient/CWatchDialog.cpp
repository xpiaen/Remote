// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialogEx)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_WATCH, pParent)
{
	m_nObjWidth = -1;
	m_nObjHeight = -1;
	m_isFull = false;
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
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK,&CWatchDialog::OnSendPacketAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point,bool isScreen)
{//800 450
	CRect clientRect;
	if (!isScreen) ClientToScreen(&point);//将客户区坐标转换为屏幕坐标（屏幕内的绝对坐标）
	m_picture.ScreenToClient(&point);//将屏幕坐标转换为客户区坐标(相对于picture坐标）
	m_picture.GetWindowRect(&clientRect);
	return CPoint((point.x * m_nObjWidth / clientRect.Width()), (((point.y * m_nObjHeight)) / clientRect.Height()));
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_isFull = false;
	//SetTimer(0, 45, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//if (nIDEvent == 0) {
	//	CClientController* pCtrl = CClientController::getInstance();
	//	if (m_isFull) {
	//		CRect rect;
	//		m_picture.GetWindowRect(rect);//获取图片的大小
	//		m_nObjWidth = m_image.GetWidth();
	//		m_nObjHeight = m_image.GetHeight();
	//		m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
	//		m_picture.InvalidateRect(NULL);
	//		m_image.Destroy();
	//		m_isFull = false;
	//		TRACE("更新图片完成%d %d\r\n", m_nObjWidth, m_nObjHeight);
	//	}
	//}
	CDialogEx::OnTimer(nIDEvent);
}


LRESULT CWatchDialog::OnSendPacketAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {
		//TODO:错误处理
	}
	else if (lParam == 1) {
		//对方关闭了套接字
	}
	else {
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch (head.sCmd) {
			case 6:
			{
				CEdoyunTools::Bytes2Image(m_image, head.strData);
				CRect rect;
				m_picture.GetWindowRect(rect);//获取图片的大小
				m_nObjWidth = m_image.GetWidth();
				m_nObjHeight = m_image.GetHeight();
				m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
				m_picture.InvalidateRect(NULL);
				m_image.Destroy();
				m_isFull = false;
				TRACE("更新图片完成%d %d\r\n", m_nObjWidth, m_nObjHeight);
				break;
			}
			case 5:
				TRACE("远程端应答鼠标事件\r\n");
				break;
			case 7:
			case 8:
			default:
				break;
			}
		}
	}
	return LRESULT();
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 1;//双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*) & event, sizeof(event));
	}
	
	CDialogEx::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//按下
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialogEx::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 3;//弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 1;//双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialogEx::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 2;//按下
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialogEx::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 3;//弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialogEx::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;//无按键
		event.nAction = 0;//移动
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialogEx::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint point;
		GetCursorPos(&point);
		CPoint remote = UserPoint2RemoteScreenPoint(point, true);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 0;//单击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialogEx::OnOK();
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);
}
