
// loadDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "load.h"
#include "loadDlg.h"
#include "afxdialogex.h"
#include <winsvc.h> 
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
BOOL LoadNTDriver(char* lpDriverName, char* lpDriverPathName);
BOOL UnLoadSys(char * szSvrName);
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
CString DriverName;

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CloadDlg 对话框



CloadDlg::CloadDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CloadDlg::IDD, pParent)
	, m_syspathname(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_EDIT_SYSPATHNAME, m_syspathname);
	DDX_Text(pDX, IDC_EDIT_SYSPATHNAME, m_syspathname);
}

BEGIN_MESSAGE_MAP(CloadDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_LOADSYS, &CloadDlg::OnBnClickedButtonLoadsys)
	ON_BN_CLICKED(IDC_BUTTON_UNLOADSYS, &CloadDlg::OnBnClickedButtonUnloadsys)
END_MESSAGE_MAP()


// CloadDlg 消息处理程序

BOOL CloadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// TODO:  在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CloadDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CloadDlg::OnPaint()
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
HCURSOR CloadDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CloadDlg::OnBnClickedButtonLoadsys()
{
	CFileDialog sysFile(true, NULL, NULL, 0, "驱动文件sys|*.sys|所有文件|*.*|");
	if (IDOK == sysFile.DoModal())
	{
		m_syspathname = sysFile.GetPathName();	//获取路径名字
		m_syspathname = sysFile.GetFileName();	//文件名字
		DriverName = sysFile.GetFileName();
		UpdateData(false);
		//LoadNtDriver;
		LoadNTDriver(sysFile.GetFileName().GetBuffer(256), sysFile.GetPathName().GetBuffer(256));
	}
}

BOOL LoadNTDriver(char* lpDriverName, char* lpDriverPathName)
{
	BOOL bRet = FALSE;

	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄

	//打开服务控制管理器
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hServiceMgr == NULL)
	{
		//OpenSCManager失败
		TRACE("OpenSCManager() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BExit;
	}
	else
	{
		////OpenSCManager成功
		TRACE("OpenSCManager() ok ! \n");
	}

	//创建驱动所对应的服务
	hServiceDDK = CreateService(hServiceMgr,
		lpDriverName, //驱动程序的在注册表中的名字  
		lpDriverName, // 注册表驱动程序的 DisplayName 值  
		SERVICE_ALL_ACCESS, // 加载驱动程序的访问权限  
		SERVICE_KERNEL_DRIVER,// 表示加载的服务是驱动程序  
		SERVICE_DEMAND_START, // 注册表驱动程序的 Start 值  
		SERVICE_ERROR_IGNORE, // 注册表驱动程序的 ErrorControl 值  
		lpDriverPathName, // 注册表驱动程序的 ImagePath 值  
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	DWORD dwRtn;
	//判断服务是否失败
	if (hServiceDDK == NULL)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS)
		{
			//由于其他原因创建服务失败
			TRACE("CrateService() 失败 %d ! \n", dwRtn);
			bRet = FALSE;
			goto BExit;
		}
		else
		{
			//服务创建失败，是由于服务已经创立过
			TRACE("CrateService() 服务创建失败，是由于服务已经创立过 ERROR is ERROR_IO_PENDING or ERROR_SERVICE_EXISTS! \n");
		}

		// 驱动程序已经加载，只需要打开  
		hServiceDDK = OpenService(hServiceMgr, lpDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			//如果打开服务也失败，则意味错误
			dwRtn = GetLastError();
			TRACE("OpenService() 失败 %d ! \n", dwRtn);
			bRet = FALSE;
			goto BExit;
		}
		else
		{
			TRACE("OpenService() 成功 ! \n");
		}
	}
	else
	{
		TRACE("CrateService() 成功 ! \n");
	}

	//开启此项服务
	bRet = StartService(hServiceDDK, NULL, NULL);
	if (!bRet)  //开启服务不成功
	{
		TRACE("StartService() 失败 服务可能已经开启%d ! \n", dwRtn);
	}
	bRet = TRUE;
	//离开前关闭句柄
BExit:
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}

//卸载按钮
void CloadDlg::OnBnClickedButtonUnloadsys()
{
	UnLoadSys(DriverName.GetBuffer(256));
}

//卸载驱动程序  
BOOL UnLoadSys(char * szSvrName)
{
	//一定义所用到的变量
	BOOL bRet = FALSE;
	SC_HANDLE hSCM = NULL;//SCM管理器的句柄,用来存放OpenSCManager的返回值
	SC_HANDLE hService = NULL;//NT驱动程序的服务句柄，用来存放OpenService的返回值
	SERVICE_STATUS SvrSta;
	//二打开SCM管理器
	hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCM == NULL)
	{
		//带开SCM管理器失败
		TRACE("OpenSCManager() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		//打开SCM管理器成功
		TRACE("OpenSCManager() ok ! \n");
	}
	//三打开驱动所对应的服务
	hService = OpenService(hSCM, szSvrName, SERVICE_ALL_ACCESS);

	if (hService == NULL)
	{
		//打开驱动所对应的服务失败 退出
		TRACE("OpenService() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		TRACE("OpenService() ok ! \n");  //打开驱动所对应的服务 成功
	}
	//四停止驱动程序，如果停止失败，只有重新启动才能，再动态加载。  
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &SvrSta))
	{
		TRACE("用ControlService() 停止驱动程序失败 错误号:%d !\n", GetLastError());
	}
	else
	{
		//停止驱动程序成功
		TRACE("用ControlService() 停止驱动程序成功 !\n");
	}
	//五动态卸载驱动服务。  
	if (!DeleteService(hService))  //TRUE//FALSE
	{
		//卸载失败
		TRACE("卸载失败:DeleteSrevice()错误号:%d !\n", GetLastError());
	}
	else
	{
		//卸载成功
		TRACE("卸载成功 !\n");

	}
	bRet = TRUE;
	//六 离开前关闭打开的句柄
BeforeLeave:
	if (hService > 0)
	{
		CloseServiceHandle(hService);
	}
	if (hSCM > 0)
	{
		CloseServiceHandle(hSCM);
	}
	return bRet;
}

