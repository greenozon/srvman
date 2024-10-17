// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"
#include "cmdline.h"
#include "PropertiesDlg.h"

#include <file.h>
#include <buffer.h>
#include <Win32/services.h>

#if !defined(BAZISLIB_VERSION) || (BAZISLIB_VERSION < 0x214)
#error BazisLib >= 2.1.4 is required to build this application
#endif

using namespace BazisLib;
using namespace BazisLib::Win32;

static struct _ColumnDescription
{
	DWORD dwMenuID;
	unsigned DefaultWidth;
	const TCHAR *pRegistryKeyName;
	bool Enabled;
} s_Columns[] = {
	{ID_VIEW_INTERNALNAME,	100, _T("InternalName"),	true},
	{ID_VIEW_STATE,			60,	 _T("State"),			true},
	{ID_VIEW_TYPE,			60,	 _T("Type"),			true},
	{ID_VIEW_DISPLAYNAME,	200, _T("DisplayName"),		true},
	{ID_VIEW_STARTTYPE,		70,	 _T("StartType"),		true},
	{ID_VIEW_BINARYFILE,	240, _T("BinaryFile"),		true},
	{ID_VIEW_ACCOUNTNAME,	200, _T("AccountName"),		false},
};

static const TCHAR tszVisibleColumnsSubkey[] = _T("VisibleColumns");
static const TCHAR tszColumnWidthsSubkey[] = _T("ColumnWidths");

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	DlgResize_Init();

	m_ListView.m_hWnd = GetDlgItem(IDC_LIST1);

	m_ListView.AddColumn(_T("Internal name"), 0);
	m_ListView.AddColumn(_T("State"), 1);
	m_ListView.AddColumn(_T("Type"), 2);
	m_ListView.AddColumn(_T("Display name"), 3);
	m_ListView.AddColumn(_T("Start type"), 4);
	m_ListView.AddColumn(_T("Executable"), 5);
	m_ListView.AddColumn(_T("Account name"), 6);

	m_MainMenu.m_hMenu = GetMenu();

	Win32::RegistryKey rkVisibleCols(m_ParamsRoot, tszVisibleColumnsSubkey);
	Win32::RegistryKey rkColumnWidths(m_ParamsRoot, tszColumnWidthsSubkey);

	for (unsigned i = 0; i < __countof(s_Columns); i++)
	{
		rkColumnWidths[s_Columns[i].pRegistryKeyName].ReadValue(&s_Columns[i].DefaultWidth);
		rkVisibleCols[s_Columns[i].pRegistryKeyName].ReadValue(&s_Columns[i].Enabled);
		m_ListView.SetColumnWidth(i, s_Columns[i].Enabled ? s_Columns[i].DefaultWidth : 0);
		m_MainMenu.CheckMenuItem(s_Columns[i].dwMenuID, (s_Columns[i].Enabled) ? MF_CHECKED : MF_UNCHECKED);
	}

	m_ParamsRoot[_T("IconDisplayMode")].ReadValue((unsigned *)&m_IconDisplayMode);

	m_ImageList.Create(16, 16, ILC_COLOR4 | ILC_MASK, 4, 0);
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_GREY)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_YELLOW)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_GREEN)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_RED)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_DRIVER)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_FSDRIVER)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_SERV)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_MULTISERV)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_INTERACTIVE)));

	m_ListView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListView.ModifyStyle(0, LVS_SINGLESEL | LVS_SORTASCENDING );

	m_ListView.SetImageList(m_ImageList, LVSIL_SMALL);
	m_ListView.SetImageList(m_ImageList, LVSIL_STATE);

	m_MainMenu.CheckMenuItem(ID_ICONMEANING_SERVICETYPE, (m_IconDisplayMode == imServiceType) ? MF_CHECKED : MF_UNCHECKED);
	m_MainMenu.CheckMenuItem(ID_ICONMEANING_SERVICESTATE, (m_IconDisplayMode == imServiceState) ? MF_CHECKED : MF_UNCHECKED);

	ReloadServiceList();

	SetTimer(0, 100);

	m_ListView.SelectItem(0);

	return bHandled = FALSE;
}

static int  g_iSortColumn;
static BOOL g_bSortAscending;
static int CALLBACK CompareFunction(LPARAM lparam1, LPARAM lparam2, LPARAM lparamsort)
{
	BSTR bstrItem1 = nullptr;
	BSTR bstrItem2 = nullptr;

	CListViewCtrl* m_lv = (CListViewCtrl*)lparamsort; ;
	m_lv->GetItemText((int)lparam1, g_iSortColumn, bstrItem1);
	m_lv->GetItemText((int)lparam2, g_iSortColumn, bstrItem2);

	int result = 0;
	if (bstrItem1 && bstrItem2)
	{
		if (g_bSortAscending)
			result = _wcsicmp(bstrItem1, bstrItem2);
		else
			result = _wcsicmp(bstrItem2, bstrItem1);
	}
	// Free the BSTRs after comparison
	SysFreeString(bstrItem1);
	SysFreeString(bstrItem2);

	return result;
}

void CMainDlg::Sort(int iColumn, BOOL bAscending)
{
	g_iSortColumn = iColumn;
	g_bSortAscending = bAscending;

	LVITEM lvItem;

	for (int n = 0; n < m_ListView.GetItemCount(); n++)
	{
		lvItem.mask = LVIF_PARAM;
		//SortItems uses the lvItem.lParam to sort the CListCtrl 
		//The lParam1 parameter is the 32-bit value associated with the first item that is 
		// compared, and lParam2 parameter is the value associated with the second 
		// item.
		// The lParam1 and lParam2 were made the sames as the current nitem numbers for the 
		// current ListCtrl table each time the column header is clicked to new sort.
		lvItem.lParam = (LPARAM)n;
		lvItem.iItem = n;
		lvItem.iSubItem = 0;

		m_ListView.SetItem(&lvItem);
	}

	m_ListView.SortItems(CompareFunction, (LPARAM)&m_ListView);
}

LRESULT CMainDlg::OnListViewColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
	LPNM_LISTVIEW pNMListView = (LPNM_LISTVIEW)pnmh;

	const int iColumn = pNMListView->iSubItem;
	//int Col = ((NM_LISTVIEW*)pNMHDR)->iSubItem;
	//m_lv->SortItems(compare, (LPARAM)&GetListCtrl());// pHeader->iItem
	
	// if it's a second click on the same column then reverse the sort order,
	// otherwise sort the new column in ascending order
	Sort(iColumn, iColumn == g_iSortColumn ? !g_bSortAscending : TRUE);

	return 0;
}


LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CSimpleDialog<IDD_ABOUTBOX, FALSE> dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnBnClickedExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SaveState();
	EndDialog(0);
	return 0;
}

LRESULT CMainDlg::OnCommandLineHelp( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	AllocConsole();
	SetConsoleOutputCP(CP_ACP);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwDone = 0;
	WriteFile(hStdOut, szCommandLineHelpMsg, (DWORD)strlen(szCommandLineHelpMsg), &dwDone, NULL);
	WriteFile(hStdOut, szPressAnyKey, (DWORD)strlen(szPressAnyKey), &dwDone, NULL);
//	INPUT_RECORD rec = {0,};
//	ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &rec, 1, &dwDone);
	char ch;
	SetConsoleTitle(_T("SrvMan Console"));
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_INSERT_MODE);
	ReadFile(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &dwDone, NULL);
	FreeConsole();
	return 0;
}

void CMainDlg::ReloadServiceList()
{
	m_ListView.DeleteAllItems();

	ServiceControlManager mgr(SC_MANAGER_ENUMERATE_SERVICE);
	TypedBuffer<QUERY_SERVICE_CONFIG> pConfig;
	for (ServiceControlManager::iterator it = mgr.begin(); it != mgr.end(); ++it)
	{
		if (!it.Valid())
			continue;
		
		int idx = m_ListView.InsertItem(-1, it->lpServiceName);

		m_ListView.SetItem(idx, 1, LVIF_TEXT, Service::GetStateName(it->ServiceStatusProcess.dwCurrentState), 0, 0, 0, 0);
		m_ListView.SetItemData(idx, idx); // set LPARAM for sorting

		m_ListView.SetItem(idx, 2, LVIF_TEXT, Service::GetServiceTypeName(it->ServiceStatusProcess.dwServiceType), 0, 0, 0, 0);
		m_ListView.SetItem(idx, 3, LVIF_TEXT, it->lpDisplayName, 0, 0, 0, 0);

		Service srv(&mgr, it->lpServiceName, SERVICE_QUERY_CONFIG);
		if (srv.QueryConfig(&pConfig).Successful())
		{
			m_ListView.SetItem(idx, 4, LVIF_TEXT, Service::GetStartTypeName(pConfig->dwStartType), 0, 0, 0, 0);
//			m_ListView.SetItem(idx, 5, LVIF_TEXT, Service::GetServiceBinaryPath(pConfig->lpBinaryPathName, false).c_str(), 0, 0, 0, 0);
			m_ListView.SetItem(idx, 5, LVIF_TEXT, pConfig->lpBinaryPathName, 0, 0, 0, 0);
			m_ListView.SetItem(idx, 6, LVIF_TEXT, pConfig->lpServiceStartName, 0, 0, 0, 0);
			UpdateServiceIcon(idx, it->ServiceStatusProcess.dwServiceType, pConfig->dwStartType, it->ServiceStatusProcess.dwCurrentState);
		}
		else
			UpdateServiceIcon(idx, it->ServiceStatusProcess.dwServiceType, 0, it->ServiceStatusProcess.dwCurrentState);
	}
}

LRESULT CMainDlg::OnViewFlagsChanged( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	Win32::RegistryKey rkVisibleCols(m_ParamsRoot, tszVisibleColumnsSubkey);
	for (unsigned i = 0; i < __countof(s_Columns); i++)
		if (s_Columns[i].dwMenuID == wID)
		{
			s_Columns[i].Enabled = !s_Columns[i].Enabled;
			m_ListView.SetColumnWidth(i, s_Columns[i].Enabled ? s_Columns[i].DefaultWidth : 0);
			m_MainMenu.CheckMenuItem(s_Columns[i].dwMenuID, (s_Columns[i].Enabled) ? MF_CHECKED : MF_UNCHECKED);
			rkVisibleCols[s_Columns[i].pRegistryKeyName] = s_Columns[i].Enabled;
			break;
		}
	return 0;
}

CMainDlg::CMainDlg()
: m_ParamsRoot(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\SysProgs\\SrvMan"))
, m_IconDisplayMode(imServiceType)
{
}

void CMainDlg::UpdateServiceIcon( unsigned Index, DWORD dwType, DWORD dwLoad, DWORD dwState )
{
	unsigned img = -1;
	switch (m_IconDisplayMode)
	{
	case imServiceType:
		switch (dwType)
		{
		case SERVICE_FILE_SYSTEM_DRIVER:
			img = icoFsDriver;
			break;
		case SERVICE_KERNEL_DRIVER:
			img = icoDriver;
			break;
		case SERVICE_WIN32_OWN_PROCESS:
			img = icoService;
			break;
		case SERVICE_WIN32_SHARE_PROCESS:
			img = icoMultiService;
			break;
		case SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS:
		case SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_SHARE_PROCESS:
			img = icoInteractive;
			break;
		}
		break;
	case imServiceState:
	default:
		switch (dwState)
		{
		case SERVICE_RUNNING:
			img = icoGreen;
			break;
		case SERVICE_STOPPED:
			if ((dwLoad == SERVICE_DISABLED) || (dwLoad == SERVICE_DEMAND_START))
				img = icoGrey;
			else
				img = icoRed;
			break;
		default:
			img = icoYellow;
			break;
		}
		break;
	}
	m_ListView.SetItem(Index, 0, LVIF_IMAGE, NULL, img, 0, 0, 0);
}

LRESULT CMainDlg::OnIconModeChanged( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	switch (wID)
	{
	case ID_ICONMEANING_SERVICETYPE:
		m_IconDisplayMode = imServiceType;
		break;
	case ID_ICONMEANING_SERVICESTATE:
		m_IconDisplayMode = imServiceState;
		break;
	}
	m_MainMenu.CheckMenuItem(ID_ICONMEANING_SERVICETYPE, (m_IconDisplayMode == imServiceType) ? MF_CHECKED : MF_UNCHECKED);
	m_MainMenu.CheckMenuItem(ID_ICONMEANING_SERVICESTATE, (m_IconDisplayMode == imServiceState) ? MF_CHECKED : MF_UNCHECKED);
	m_ParamsRoot[_T("IconDisplayMode")] = (unsigned)m_IconDisplayMode;
	UpdateServiceStates();
	return 0;
}

void CMainDlg::UpdateServiceStates()
{
	ServiceControlManager mgr(SC_MANAGER_ENUMERATE_SERVICE);
	for (int i = 0; i < m_ListView.GetItemCount(); i++)
		UpdateServiceInfo(&mgr, i, false);
}

LRESULT CMainDlg::OnTimer( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
	int firstIdx   = m_ListView.GetTopIndex();
	int lastIdx	   = firstIdx + m_ListView.GetCountPerPage();
	if (lastIdx >= m_ListView.GetItemCount())
		lastIdx = m_ListView.GetItemCount() - 1;

	ServiceControlManager mgr(SC_MANAGER_ENUMERATE_SERVICE);
	for (int i = firstIdx; i <= lastIdx; i++)
		UpdateServiceInfo(&mgr, i, false);

	OnSelChanged(0, 0, *(BOOL *)0);

	if (!m_RestartPendingServiceName.empty())
	{
		ServiceControlManager mgr;
		Service srv(&mgr, m_RestartPendingServiceName.c_str(), SERVICE_QUERY_STATUS | SERVICE_START);
		SERVICE_STATUS_PROCESS srvStatus;
		ActionStatus st = srv.QueryStatus(&srvStatus);
		if (!st.Successful())
		{
			MessageBox((String(_T("Cannot open service object ")) + m_RestartPendingServiceName + _T(": ") + st.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
			return 0;
		}
		if (srvStatus.dwCurrentState != SERVICE_STOPPED)
			return 0;
		st = srv.Start();
		if (!st.Successful())
		{
			MessageBox((String(_T("Cannot start service ")) + m_RestartPendingServiceName + _T(": ") + st.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
			return 0;
		}
		m_RestartPendingServiceName.clear();
	}
	//Nothing goes here, as service restart code can return control
	return 0;
}

void CMainDlg::UpdateServiceInfo(class BazisLib::Win32::ServiceControlManager *pMgr, unsigned Index, bool UpdateExtendedInfo )
{
	TCHAR tsz[512];
	TypedBuffer<QUERY_SERVICE_CONFIG> pConfig;
	if (!m_ListView.GetItemText(Index, 0, tsz, __countof(tsz)))
		return;
	SERVICE_STATUS_PROCESS status;
	Service srv(pMgr, tsz, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
	if (!srv.QueryStatus(&status).Successful())
		return;
	unsigned startType = 0;
	if (srv.QueryConfig(&pConfig).Successful())
		startType = pConfig->dwStartType;

	UpdateServiceIcon(Index, status.dwServiceType, startType, status.dwCurrentState);
	SetListItemText(Index, 1, Service::GetStateName(status.dwCurrentState));

	if (UpdateExtendedInfo)
	{
		SetListItemText(Index, 2, Service::GetServiceTypeName(status.dwServiceType));
		SetListItemText(Index, 3, pConfig->lpDisplayName);
		SetListItemText(Index, 4, Service::GetStartTypeName(pConfig->dwStartType));
		SetListItemText(Index, 5, pConfig->lpBinaryPathName);
		SetListItemText(Index, 6, pConfig->lpServiceStartName);
	}
}

void CMainDlg::SetListItemText( unsigned Index, unsigned Subindex, LPCTSTR pszText )
{
	TCHAR tsz[512];
	ASSERT(pszText);
	if (m_ListView.GetItemText(Index, Subindex, tsz, __countof(tsz)))
		if (!_tcscmp(pszText, tsz))
			return;
	m_ListView.SetItemText(Index, Subindex, pszText);
}

LRESULT CMainDlg::OnClose( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
	SaveState();
	EndDialog(0);
	return 0;
}

void CMainDlg::SaveState()
{
	Win32::RegistryKey rkColumnWidths(m_ParamsRoot, tszColumnWidthsSubkey);
	for (unsigned i = 0; i < __countof(s_Columns); i++)
		if (s_Columns[i].Enabled)
			rkColumnWidths[s_Columns[i].pRegistryKeyName] = m_ListView.GetColumnWidth(i);
}

LRESULT CMainDlg::OnSelChanged( int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/ )
{
	Service srv;
	if (!OpenSelectedService(&srv, SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS).Successful())
		return 0;
	SERVICE_STATUS_PROCESS proc;
	TypedBuffer<QUERY_SERVICE_CONFIG> pConfig;
	if (srv.QueryStatus(&proc).Successful() && srv.QueryConfig(&pConfig).Successful())
	{
		bool enableStart = false, enableStop = false, enableRestart = false;
		SetDlgItemText(IDC_RESTART, _T("Restart service"));
		switch(proc.dwCurrentState)
		{
		case SERVICE_RUNNING:
			enableStop = enableRestart = true;
			SetDlgItemText(IDC_STARTSTOP, _T("Stop service"));
			break;
		case SERVICE_STOPPED:
			enableStart = true;
			SetDlgItemText(IDC_STARTSTOP, _T("Start service"));
			/*if (pConfig->dwStartType == SERVICE_DISABLED)
			{
				enableRestart = true;
				SetDlgItemText(IDC_RESTART, _T("Enable service"));
			}*/
			break;
		default:
			break;
		}

		if (!m_RestartPendingServiceName.empty())
			enableRestart = false;

		::EnableWindow(GetDlgItem(IDC_STARTSTOP), enableStart || enableStop);
		::EnableWindow(GetDlgItem(IDC_RESTART), enableRestart);
		m_MainMenu.EnableMenuItem(ID_CONTROL_START, enableStart ? MF_ENABLED : MF_GRAYED);
		m_MainMenu.EnableMenuItem(ID_CONTROL_STOP,	enableStop ? MF_ENABLED : MF_GRAYED);
		m_MainMenu.EnableMenuItem(ID_CONTROL_RESTART, enableRestart ? MF_ENABLED : MF_GRAYED);
	}

	return 0;
}

LRESULT CMainDlg::OnOpenWebpage( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	ShellExecute(m_hWnd, _T("open"), _T("http://tools.sysprogs.org/srvman"), NULL, NULL, SW_SHOW);
	return 0;
}

LRESULT CMainDlg::StartStopService( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	Service srv;
	String srvName;
	ActionStatus st = OpenSelectedService(&srv, SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP, &srvName);
	enum {doStart, doStop, doRestart} actionToDo;
	if (!st.Successful())
	{
		MessageBox((String(_T("Cannot open service object: ")) + st.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
		return 0;
	}

	switch (wID)
	{
	case ID_CONTROL_RESTART:
	case IDC_RESTART:
		actionToDo = doRestart;
		break;
	case ID_CONTROL_START:
		actionToDo = doStart;
		break;
	case ID_CONTROL_STOP:
		actionToDo = doStop;
		break;
	case IDC_STARTSTOP:
		{
			SERVICE_STATUS_PROCESS srvStatus;
			st = srv.QueryStatus(&srvStatus);
			if (!st.Successful())
			{
				MessageBox((String(_T("Cannot query service: ")) + st.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
				return 0;
			}
			if (srvStatus.dwCurrentState == SERVICE_RUNNING)
				actionToDo = doStop;
			else
				actionToDo = doStart;
		}
		break;
	}

	switch (actionToDo)
	{
	case doStart:
		st = srv.Start();
		break;
	case doStop:
		st = srv.Stop();
		break;
	case doRestart:
		st = srv.Stop();
		m_RestartPendingServiceName = srvName;
		break;
	}

	if (!st.Successful())
	{
		MessageBox((String(_T("Cannot control service: ")) + st.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
		return 0;
	}

	return 0;
}

ActionStatus CMainDlg::OpenSelectedService(BazisLib::Win32::Service *pService, DWORD dwAccess /*= SERVICE_ALL_ACCESS*/, String *pName )
{
	int idx = m_ListView.GetSelectedIndex();
	if (idx == -1)
		return MAKE_STATUS(UnknownError);

	TCHAR tsz[512];
	if (!m_ListView.GetItemText(idx, 0, tsz, __countof(tsz)))
		return MAKE_STATUS(UnknownError);

	if (pName)
		*pName = tsz;

	ServiceControlManager mgr(SC_MANAGER_ENUMERATE_SERVICE);
	return pService->OpenAnotherService(&mgr, tsz, dwAccess);
}

LRESULT CMainDlg::OnRefreshSelected( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	ReloadServiceList();
	return 0;
}

LRESULT CMainDlg::OnBnClickedProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	Service srv;
	bool ReadOnly = false;
	String srvName;
	ActionStatus st = OpenSelectedService(&srv, SERVICE_ALL_ACCESS, &srvName);
	if (st.GetErrorCode() == BazisLib::CommonErrorType::AccessDenied)
		st = OpenSelectedService(&srv, SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS), ReadOnly = true;
	if (!st.Successful())
	{
		MessageBox((String(_T("Cannot open service object ")) + m_RestartPendingServiceName + _T(": ") + st.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
		return 0;
	}

	CPropertiesDlg dlg(srvName, &srv, ReadOnly);
	dlg.DoModal();

	ServiceControlManager mgr(SC_MANAGER_ENUMERATE_SERVICE);
	int idx = m_ListView.GetSelectedIndex();
	if (idx != -1)
		UpdateServiceInfo(&mgr, idx, true);

	return 0;
}

LRESULT CMainDlg::OnListViewDblClick( int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled )
{
	return OnBnClickedProperties(0, 0, 0, bHandled);
}

LRESULT CMainDlg::OnBnClickedAddservice(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CPropertiesDlg dlg;
	if (dlg.DoModal() == IDOK)
		ReloadServiceList();
	return 0;
}

LRESULT CMainDlg::OnBnClickedDeleteservice(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (MessageBox(_T("Do you want to delete the selected service?"), _T("Question"), MB_ICONQUESTION | MB_YESNO) == IDYES)
	{
		Service svc;
		OpenSelectedService(&svc, DELETE);
		ActionStatus st = svc.Delete();
		if (!st.Successful())
		{
			MessageBox((String(_T("Cannot delete service: ")) + m_RestartPendingServiceName + _T(": ") + st.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
			return 0;
		}
		m_ListView.DeleteItem(m_ListView.GetSelectedIndex());
	}
	return 0;
}
