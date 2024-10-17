#include "stdafx.h"
#include "PropertiesDlg.h"
#include <bzs_string.h>

using namespace BazisLib;
using namespace BazisLib::Win32;

CPropertiesDlg::CPropertiesDlg(const String &Name, Service *pService, bool ReadOnly)
	: m_pService(pService)
	, m_bReadOnly(ReadOnly)
	, m_Name(Name)
	, m_bLastChangedPathIsRawPath(true)
	, m_ValueChangeFlags(vcfNone)
	, m_bDoNotUpdatePaths(false)
	, m_bNonServiceExe(false)
{
}

LRESULT CPropertiesDlg::OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled )
{
	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	DlgResize_Init();

	m_ImageList.Create(16, 16, ILC_COLOR4 | ILC_MASK, 4, 0);
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_DRIVER)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_FSDRIVER)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_SERV)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_MULTISERV)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME)));
	
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_AUTO)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_BOOT)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_MANUAL)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_DISABLED)));
	m_ImageList.AddIcon(LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_SYSTEM)));
	
	m_cbServiceType.m_hWnd = GetDlgItem(IDC_SERVICETYPE);
	m_cbStartMode.m_hWnd = GetDlgItem(IDC_STARTMODE);

	m_cbServiceType.SetImageList(m_ImageList.m_hImageList);
	m_cbStartMode.SetImageList(m_ImageList.m_hImageList);
	
	COMBOBOXEXITEM item = {0, };
	item.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	item.pszText = _T("Filesystem driver"),		item.iImage = item.iSelectedImage = icoFsDriver,		m_cbServiceType.InsertItem(&item), item.iItem++;
	item.pszText = _T("Device driver"),			item.iImage = item.iSelectedImage = icoDriver,			m_cbServiceType.InsertItem(&item), item.iItem++;
	item.pszText = _T("Win32 own process"),		item.iImage = item.iSelectedImage = icoService,			m_cbServiceType.InsertItem(&item), item.iItem++;
	item.pszText = _T("Win32 shared process"),	item.iImage = item.iSelectedImage = icoMultiService,	m_cbServiceType.InsertItem(&item), item.iItem++;
	item.pszText = _T("Win32 program as service via SrvMan"),	item.iImage = item.iSelectedImage = icoSysprogs,		m_cbServiceType.InsertItem(&item), item.iItem++;

	item.iItem = 0;
	item.pszText = _T("Auto (Started by Service Control Manager during startup)"),			item.iImage = item.iSelectedImage = icoAuto,		m_cbStartMode.InsertItem(&item), item.iItem++;
	item.pszText = _T("Boot (Started by System Loader)"),									item.iImage = item.iSelectedImage = icoBoot,		m_cbStartMode.InsertItem(&item), item.iItem++;
	item.pszText = _T("Manual (Started via StartService() or \"net start\")"),				item.iImage = item.iSelectedImage = icoManual,		m_cbStartMode.InsertItem(&item), item.iItem++;
	item.pszText = _T("Disabled (Service cannot be started)"),								item.iImage = item.iSelectedImage = icoDisabled,	m_cbStartMode.InsertItem(&item), item.iItem++;
	item.pszText = _T("System (Started by IoInitSystem() call)"),							item.iImage = item.iSelectedImage = icoSystem,		m_cbStartMode.InsertItem(&item), item.iItem++;

	if (m_bReadOnly)
	{
		::EnableWindow(GetDlgItem(IDC_INTERNALNAME), FALSE);
		::EnableWindow(GetDlgItem(IDC_VISIBLENAME), FALSE);
		::EnableWindow(GetDlgItem(IDC_WIN32PATH), FALSE);
		::EnableWindow(GetDlgItem(IDC_TRANSLATEDPATH), FALSE);
		::EnableWindow(GetDlgItem(IDC_BROWSE), FALSE);
		::EnableWindow(GetDlgItem(IDC_SERVICETYPE), FALSE);
		::EnableWindow(GetDlgItem(IDC_INTERACTIVE), FALSE);
		::EnableWindow(GetDlgItem(IDC_STARTMODE), FALSE);
		::EnableWindow(GetDlgItem(IDC_LOADGROUP), FALSE);
		::EnableWindow(GetDlgItem(IDC_LOGIN), FALSE);
		::EnableWindow(GetDlgItem(IDC_PASSWORD), FALSE);
		::EnableWindow(GetDlgItem(IDC_DESCRIPTION), FALSE);
		::EnableWindow(GetDlgItem(IDOK), FALSE);
	}

	if (!m_Name.empty())
	{
		SetDlgItemText(IDC_INTERNALNAME, m_Name.c_str());
		SendDlgItemMessage(IDC_INTERNALNAME, EM_SETREADONLY, TRUE, 0);
	}

	if (m_pService)
	{
		TypedBuffer<QUERY_SERVICE_CONFIG> pConfig;
		if (m_pService->QueryConfig(&pConfig).Successful())
		{
			SetDlgItemText(IDC_VISIBLENAME, pConfig->lpDisplayName);
			SetDlgItemText(IDC_LOADGROUP, pConfig->lpLoadOrderGroup);
			SetDlgItemText(IDC_LOGIN, pConfig->lpServiceStartName);

			unsigned realCmdLineOffset = 0;
			if (pConfig->dwServiceType & SERVICE_WIN32_OWN_PROCESS)
				realCmdLineOffset = GetNonServiceProcessCommandLine(pConfig->lpBinaryPathName);

			if (realCmdLineOffset)
				m_cbServiceType.SetCurSel(stNonServiceExe), m_bNonServiceExe;
			else
			{
				switch (pConfig->dwServiceType)
				{
				case SERVICE_FILE_SYSTEM_DRIVER:
					m_cbServiceType.SetCurSel(stFsDriver);
					break;
				case SERVICE_KERNEL_DRIVER:
					m_cbServiceType.SetCurSel(stKernelDriver);
					break;
				case SERVICE_WIN32_OWN_PROCESS:
				case SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS:
					m_cbServiceType.SetCurSel(stOwnProcess);
					break;
				case SERVICE_WIN32_SHARE_PROCESS:
				case SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS:
					m_cbServiceType.SetCurSel(stSharedProcess);
					break;
				}
			}

			SendDlgItemMessage(IDC_INTERACTIVE, BM_SETCHECK, (pConfig->dwServiceType & SERVICE_INTERACTIVE_PROCESS) ? BST_CHECKED : BST_UNCHECKED);

			switch (pConfig->dwStartType)
			{
			case SERVICE_AUTO_START:
				m_cbStartMode.SetCurSel(smAuto);
				break;
			case SERVICE_BOOT_START:
				m_cbStartMode.SetCurSel(smBoot);
				break;
			case SERVICE_DEMAND_START:
				m_cbStartMode.SetCurSel(smManual);
				break;
			case SERVICE_DISABLED:
				m_cbStartMode.SetCurSel(smDisabled);
				break;
			case SERVICE_SYSTEM_START:
				m_cbStartMode.SetCurSel(smSystem);
				break;
			}

			TypedBuffer<SERVICE_DESCRIPTION> pDesc;
			if (m_pService->QueryConfig2(&pDesc).Successful())
				SetDlgItemText(IDC_DESCRIPTION, pDesc->lpDescription);

			SetDlgItemText(IDC_TRANSLATEDPATH, pConfig->lpBinaryPathName + realCmdLineOffset);
			OnRawPathChanged(0, 0, 0, *(BOOL *)NULL);

			m_bDoNotUpdatePaths = true;
			OnServiceTypeChanged(0, 0, 0, *(BOOL *)NULL);
			m_bDoNotUpdatePaths = false;
		}
	}

	m_ValueChangeFlags = vcfNone;
	return bHandled = FALSE;
}

LRESULT CPropertiesDlg::OnBnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(IDCANCEL);
	return 0;
}

String BuildCommandLineForAppAsService(LPCTSTR lpOriginalCmdLine, LPCTSTR lpServiceName)
{
	TCHAR tsz[MAX_PATH + 1] = {'\"', };
	GetModuleFileName(GetModuleHandle(0), tsz + 1, __countof(tsz) - 1);
	String str = tsz;
	str += _T("\" /RunAsService:");
	str += lpServiceName;
	str += _T(" ");
	str += lpOriginalCmdLine;
	return str;
}

LRESULT CPropertiesDlg::OnBnClickedOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ServiceType st = (ServiceType)m_cbServiceType.GetCurSel();
	StartMode sm = (StartMode)m_cbStartMode.GetCurSel();
	if ((st != stFsDriver) && (st != stKernelDriver))
	{
		if ((sm == smBoot) || (sm == smSystem))
		{
			MessageBox(_T("Only drivers can have a \"Boot\" or \"System\" start mode"), _T("Error"), MB_ICONERROR);
			return 0;
		}
	}
	if (!::GetWindowTextLength(GetDlgItem(IDC_INTERNALNAME)))
	{
		MessageBox(_T("Internal service name should be specified"), _T("Error"), MB_ICONERROR);
		return 0;
	}
	if ((m_cbStartMode.GetCurSel() == -1) || (m_cbServiceType.GetCurSel() == -1))
	{
		MessageBox(_T("Service type and start mode should be selected"), _T("Error"), MB_ICONERROR);
		return 0;
	}

	TypedBuffer<TCHAR> internalName, displayName, binPath, loadGroup, loginName, loginPassword, svcDescription;
	bool creatingService = m_Name.empty() || !m_pService;
	bool nonServiceExe = false;
	DWORD mappedServiceType, mappedStartMode;

	if (!m_bNonServiceExe)
		m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfTypeChanged);

	GetDlgItemTextToTypedBuffer(IDC_INTERNALNAME, &internalName);
	if (creatingService || (m_ValueChangeFlags & vcfVisibleNameChanged))
		GetDlgItemTextToTypedBuffer(IDC_VISIBLENAME, &displayName);
	if (creatingService || (m_ValueChangeFlags & (vcfPathChanged | vcfTypeChanged)))
		GetDlgItemTextToTypedBuffer(IDC_TRANSLATEDPATH, &binPath);
	if (creatingService || (m_ValueChangeFlags & vcfGroupChanged))
		GetDlgItemTextToTypedBuffer(IDC_LOADGROUP, &loadGroup);
	if (creatingService || (m_ValueChangeFlags & vcfLoginChanged))
		GetDlgItemTextToTypedBuffer(IDC_LOGIN, &loginName);
	if (creatingService)
		GetDlgItemTextToTypedBuffer(IDC_PASSWORD, &loginPassword);
	if (creatingService || (m_ValueChangeFlags & vcfDescChanged))
		GetDlgItemTextToTypedBuffer(IDC_DESCRIPTION, &svcDescription);

	if (creatingService || (m_ValueChangeFlags & vcfTypeChanged))
	{
		switch ((ServiceType)m_cbServiceType.GetCurSel())
		{
		case stKernelDriver:
			mappedServiceType = SERVICE_KERNEL_DRIVER;
			break;
		case stFsDriver:
			mappedServiceType = SERVICE_FILE_SYSTEM_DRIVER;
			break;
		case stNonServiceExe:
			nonServiceExe = true;
			//no break
		case stOwnProcess:
			mappedServiceType = SERVICE_WIN32_OWN_PROCESS;
			if (SendDlgItemMessage(IDC_INTERACTIVE, BM_GETCHECK) & BST_CHECKED)
				mappedServiceType |= SERVICE_INTERACTIVE_PROCESS;
			break;
		case stSharedProcess:
			mappedServiceType = SERVICE_WIN32_SHARE_PROCESS;
			if (SendDlgItemMessage(IDC_INTERACTIVE, BM_GETCHECK) & BST_CHECKED)
				mappedServiceType |= SERVICE_INTERACTIVE_PROCESS;
			break;
		default:
			return 0;
		}
	}

	if (creatingService || (m_ValueChangeFlags & vcfStartChanged))
	{
		switch((StartMode)m_cbStartMode.GetCurSel())
		{
		case smBoot:
			mappedStartMode = SERVICE_BOOT_START;
			break;
		case smAuto:
			mappedStartMode = SERVICE_AUTO_START;
			break;
		case smSystem:
			mappedStartMode = SERVICE_SYSTEM_START;
			break;
		case smManual:
			mappedStartMode = SERVICE_DEMAND_START;
			break;
		case smDisabled:
			mappedStartMode = SERVICE_DISABLED;
			break;
		default:
			return 0;
		}
	}

	if (nonServiceExe)
	{
		String str = BuildCommandLineForAppAsService(binPath, internalName);
		binPath.EnsureSizeAndSetUsed((str.length() + 1) * sizeof(TCHAR));
		memcpy(binPath.GetData(), str.c_str(), binPath.GetSize());
	}

	unsigned binPathOff = 0;
	if (!_tcsncmp(binPath, _T("\\??\\"), 4))
		binPathOff = 4;

	ActionStatus status;
	Service newService;
	if (!m_pService)
	{
		ServiceControlManager mgr(SC_MANAGER_CREATE_SERVICE, NULL, &status);
		if (status.Successful())
		{
			status = mgr.CreateService(&newService,
									   internalName,
									   ((TCHAR *)displayName)[0] ? (TCHAR *)displayName : NULL,
									   SERVICE_ALL_ACCESS,
									   mappedServiceType,
									   mappedStartMode,
									   SERVICE_ERROR_NORMAL,
									   binPath + binPathOff,
									   ((TCHAR *)loadGroup)[0] ? (TCHAR *)loadGroup : NULL,
									   NULL,
									   NULL,
									   ((TCHAR *)loginName)[0] ? (TCHAR *)loginName : NULL,
									   ((TCHAR *)loginPassword)[0] ? (TCHAR *)loginPassword : NULL);
		}
	}
	else
		status = m_pService->ChangeConfig((m_ValueChangeFlags & vcfTypeChanged) ? mappedServiceType : SERVICE_NO_CHANGE,
									  (m_ValueChangeFlags & vcfStartChanged) ? mappedStartMode : SERVICE_NO_CHANGE,
									  SERVICE_NO_CHANGE,
									  (m_ValueChangeFlags & vcfPathChanged) ? ((TCHAR *)binPath + binPathOff) : NULL,
									  (m_ValueChangeFlags & vcfGroupChanged) ? (TCHAR *)loadGroup : NULL,
									  NULL,
									  NULL,
									  (m_ValueChangeFlags & vcfLoginChanged) ? (TCHAR *)loginName: NULL,
									  (loginPassword && ((TCHAR *)loginPassword)[0]) ? (TCHAR *)loginPassword : NULL,
									  (m_ValueChangeFlags & vcfVisibleNameChanged) ? (TCHAR *)displayName : NULL);

	if (!status.Successful())
	{
		if (creatingService)
			MessageBox((String(_T("Cannot create service: ")) + status.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
		else
			MessageBox((String(_T("Cannot update service: ")) + status.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONERROR);
		return 0;
	}

	if (creatingService)
		status = newService.SetDescription(svcDescription);
	else
		status = m_pService->SetDescription(svcDescription);

	if (!status.Successful())
		MessageBox((String(_T("Cannot set service description: ")) + status.GetMostInformativeText()).c_str(), _T("Service Manager"), MB_ICONWARNING);

	EndDialog(IDOK);
	return 0;
}

LRESULT CPropertiesDlg::OnClose( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled )
{
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT CPropertiesDlg::OnBnClickedProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR tsz[2048] = {0,};
	GetDlgItemText(IDC_WIN32PATH, tsz, __countof(tsz));

	SHELLEXECUTEINFO info = {0,};
	info.cbSize = sizeof(info);
	info.lpVerb = _T("properties");
	info.lpFile = tsz;

	int argc = 0;
	LPWSTR *lpArgv = CommandLineToArgvW(tsz, &argc);
	if (lpArgv)
		info.lpFile = lpArgv[0];

	info.fMask = SEE_MASK_INVOKEIDLIST;
	ShellExecuteEx(&info);
	LocalFree(lpArgv);
	return 0;
}

LRESULT CPropertiesDlg::OnServiceTypeChanged( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	ServiceType type = (ServiceType)m_cbServiceType.GetCurSel();
	bool bIsDriver = false;
	switch (type)
	{
	case stFsDriver:
	case stKernelDriver:
		bIsDriver = true;
		break;
	}
	::EnableWindow(GetDlgItem(IDC_INTERACTIVE), !bIsDriver);
	::EnableWindow(GetDlgItem(IDC_PASSWORD), !bIsDriver);
	::EnableWindow(GetDlgItem(IDC_LOGIN), !bIsDriver);

/*	if (m_bLastChangedPathIsRawPath)
		OnRawPathChanged(0, 0, 0, *(BOOL *)NULL);
	else
		OnWin32PathChanged(0, 0, 0, *(BOOL *)NULL);*/

	OnWin32PathChanged(0, 0, 0, *(BOOL *)NULL);
	m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfTypeChanged);

	return 0;
}
LRESULT CPropertiesDlg::OnBnClickedBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR tsz[2048] = {0,};
	TCHAR tszBinary[MAX_PATH] = {0,};
	GetDlgItemText(IDC_WIN32PATH, tsz, __countof(tsz));

	unsigned argsOffset = 0;
	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(tsz, &argc);
	if (argv)
	{
		argsOffset = (unsigned)wcslen(argv[0]);
		_tcsncpy_s(tszBinary, argv[0], __countof(tszBinary));
	}
	else
		_tcsncpy_s(tszBinary, tsz, __countof(tszBinary));
	LocalFree(argv);

	CFileDialog dlg(TRUE, NULL, tszBinary, OFN_HIDEREADONLY, _T("Service files (*.exe;*.sys)\0*.exe;*.sys\0Driver files (*.sys)\0*.sys\0Executable files (*.exe)\0*.exe\0All files(*.*)\0*.*\0\0"), m_hWnd);
	if (dlg.DoModal() != IDOK)
		return 0;

	String str = dlg.m_szFileName;
	if (argsOffset)
		str += (tsz + argsOffset);

	SetDlgItemText(IDC_WIN32PATH, str.c_str());

	if (!::GetWindowTextLength(GetDlgItem(IDC_INTERNALNAME)))
	{
		FilePath fp(str.c_str());
		SetDlgItemText(IDC_INTERNALNAME, fp.GetFileNameBase().c_str());
		String ext = fp.GetExtension();
		if (m_cbServiceType.GetCurSel() == -1)
		{
			if (!_tcsicmp(ext.c_str(), _T("sys")))
				m_cbServiceType.SetCurSel(stKernelDriver);
			else
				m_cbServiceType.SetCurSel(stOwnProcess);
		}
		if (m_cbStartMode.GetCurSel() == -1)
			m_cbStartMode.SetCurSel(smManual);
	}

	OnWin32PathChanged(0, 0, 0, *(BOOL *)NULL);

	return 0;
}

LRESULT CPropertiesDlg::OnWin32PathChanged( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	if (m_bDoNotUpdatePaths)
		return 0;
	m_bLastChangedPathIsRawPath = false;
	m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfPathChanged);

	TCHAR tsz[2048] = {0,};
	GetDlgItemText(IDC_WIN32PATH, tsz, __countof(tsz));
	ServiceType st = (ServiceType)m_cbServiceType.GetCurSel();
	//m_PathChangesToIgnore++;
	m_bDoNotUpdatePaths = true;

	if ((st == stKernelDriver) || (st == stFsDriver))
		SetDlgItemText(IDC_TRANSLATEDPATH, Service::Win32PathToDriverPath(tsz).c_str());
	else
		SetDlgItemText(IDC_TRANSLATEDPATH, tsz);
	//m_PathChangesToIgnore = 0;
	m_bDoNotUpdatePaths = false;
	return 0;
}

LRESULT CPropertiesDlg::OnRawPathChanged( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
	if (m_bDoNotUpdatePaths)
		return 0;
	m_bLastChangedPathIsRawPath = true;
	m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfPathChanged);

	TCHAR tsz[2048] = {0,};
	GetDlgItemText(IDC_TRANSLATEDPATH, tsz, __countof(tsz));

	ServiceType st = (ServiceType)m_cbServiceType.GetCurSel();
	//m_PathChangesToIgnore++;
	m_bDoNotUpdatePaths = true;

	if ((st == stKernelDriver) || (st == stFsDriver))
	{
		if (!tsz[0])
		{
			TCHAR tszSysDir[MAX_PATH];
			unsigned off = GetSystemDirectory(tszSysDir, __countof(tszSysDir));
			GetDlgItemText(IDC_INTERNALNAME, tsz, __countof(tsz));
			_sntprintf_s(tszSysDir + off, __countof(tszSysDir) - off, __countof(tszSysDir) - off, _T("\\drivers\\%s.sys"), tsz);
			SetDlgItemText(IDC_WIN32PATH, tszSysDir);
			m_bDoNotUpdatePaths = false;
			return 0;
		}

		SetDlgItemText(IDC_WIN32PATH, Service::TranslateDriverPath(tsz).c_str());
	}
	else
		SetDlgItemText(IDC_WIN32PATH, tsz);
	//m_PathChangesToIgnore = 0;
	m_bDoNotUpdatePaths = false;
	return 0;
}

void CPropertiesDlg::GetDlgItemTextToTypedBuffer( unsigned ID, BazisLib::TypedBuffer<TCHAR> *pBuf )
{
	ASSERT(pBuf);
	if (!pBuf->EnsureSizeAndSetUsed((::GetWindowTextLength(GetDlgItem(ID)) + 1) * sizeof(TCHAR)))
		return;
	GetDlgItemText(ID, *pBuf, (int)pBuf->GetAllocated());
}

LRESULT CPropertiesDlg::OnParamChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	switch (wID)
	{
	case IDC_VISIBLENAME:
		m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfVisibleNameChanged);
		break;
	case IDC_INTERACTIVE:
		m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfTypeChanged);
		break;
	case IDC_LOADGROUP:
		m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfGroupChanged);
		break;
	case IDC_LOGIN:
		m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfLoginChanged);
		break;
	case IDC_DESCRIPTION:
		m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfDescChanged);
		break;
	case IDC_STARTMODE:
		m_ValueChangeFlags = (ValueChangeFlags)(m_ValueChangeFlags | vcfStartChanged);
		break;
	}
	return 0;
}

unsigned CPropertiesDlg::GetNonServiceProcessCommandLine( LPCTSTR lpSrvManCommandLine )
{
	if (!lpSrvManCommandLine || !lpSrvManCommandLine[0])
		return 0;
	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(lpSrvManCommandLine, &argc);
	if (!argv)
		return 0;
	unsigned off = 0;
	FilePath fp = argv[0];
	if ((argc >= 2) && !_tcsicmp(fp.GetFileNameBase().c_str(), _T("srvman")) && !_tcsnicmp(argv[1], _T("/RunAsService:"), 14))
	{
		LPCTSTR p = _tcsstr(lpSrvManCommandLine, argv[1]);
		if (p)
			off = (unsigned)((p - lpSrvManCommandLine) + _tcslen(argv[1]) + 1);
	}
	LocalFree(argv);
	return off;
}
