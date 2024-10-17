// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bzswin/registry.h>
#include <bzswin/services.h>

class CMainDlg : public CDialogImpl<CMainDlg>, public CDialogResize<CMainDlg>
{
private:
	CListViewCtrl m_ListView;
	CImageList    m_ImageList;
	CMenuHandle   m_MainMenu;
	BazisLib::Win32::RegistryKey m_ParamsRoot;
	enum
	{
		imServiceType,
		imServiceState
	}			  m_IconDisplayMode;
	BazisLib::String m_RestartPendingServiceName;

public:
	enum { IDD = IDD_MAINDLG };

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		COMMAND_HANDLER(IDC_EXIT, BN_CLICKED, OnBnClickedExit)
		COMMAND_HANDLER(IDC_PROPERTIES, BN_CLICKED, OnBnClickedProperties)
		COMMAND_HANDLER(ID_SERVICE_PROPERTIES, BN_CLICKED, OnBnClickedProperties)
		NOTIFY_HANDLER(IDC_LIST1, NM_DBLCLK, OnListViewDblClick)
		COMMAND_HANDLER(IDC_ADDSERVICE, BN_CLICKED, OnBnClickedAddservice)
		COMMAND_HANDLER(ID_SERVICE_ADDSERVICE, BN_CLICKED, OnBnClickedAddservice)
		COMMAND_HANDLER(IDC_DELETESERVICE, BN_CLICKED, OnBnClickedDeleteservice)
		COMMAND_HANDLER(ID_SERVICE_DELETESERVICE, BN_CLICKED, OnBnClickedDeleteservice)
		CHAIN_MSG_MAP(CDialogResize)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_HELP_COMMANDLINEHELP, OnCommandLineHelp)
		COMMAND_ID_HANDLER(ID_VIEW_INTERNALNAME,	OnViewFlagsChanged)
		COMMAND_ID_HANDLER(ID_VIEW_STATE,			OnViewFlagsChanged)
		COMMAND_ID_HANDLER(ID_VIEW_TYPE,			OnViewFlagsChanged)
		COMMAND_ID_HANDLER(ID_VIEW_DISPLAYNAME,		OnViewFlagsChanged)
		COMMAND_ID_HANDLER(ID_VIEW_STARTTYPE,		OnViewFlagsChanged)
		COMMAND_ID_HANDLER(ID_VIEW_BINARYFILE,		OnViewFlagsChanged)
		COMMAND_ID_HANDLER(ID_VIEW_ACCOUNTNAME,		OnViewFlagsChanged)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH,			OnRefreshSelected)
		COMMAND_ID_HANDLER(ID_ICONMEANING_SERVICETYPE,		OnIconModeChanged)
		COMMAND_ID_HANDLER(ID_ICONMEANING_SERVICESTATE,		OnIconModeChanged)
		COMMAND_ID_HANDLER(ID_HELP_OPENPROJECTPAGE,	OnOpenWebpage)
		NOTIFY_HANDLER(IDC_LIST1, LVN_ITEMCHANGED, OnSelChanged)
		COMMAND_ID_HANDLER(IDC_STARTSTOP,		StartStopService)
		COMMAND_ID_HANDLER(ID_CONTROL_START,	StartStopService)
		COMMAND_ID_HANDLER(ID_CONTROL_STOP,		StartStopService)
		COMMAND_ID_HANDLER(IDC_RESTART,			StartStopService)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_LIST1, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_PROPERTIES, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STARTSTOP, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_RESTART, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_ADDSERVICE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_DELETESERVICE, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_EXIT, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	CMainDlg();

private:
	void ReloadServiceList();
	void UpdateServiceIcon(unsigned Index, DWORD dwType, DWORD dwLoad, DWORD dwState);

	void UpdateServiceStates();
	void UpdateServiceInfo(BazisLib::Win32::ServiceControlManager *pMgr, unsigned Index, bool UpdateExtendedInfo);

	void SetListItemText(unsigned Index, unsigned Subindex, LPCTSTR pszText);

	void SaveState();

	BazisLib::ActionStatus OpenSelectedService(BazisLib::Win32::Service *pService, DWORD dwAccess = SERVICE_ALL_ACCESS, BazisLib::String *pName = NULL);

public:
// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCommandLineHelp(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
	LRESULT OnViewFlagsChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRefreshSelected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnIconModeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOpenWebpage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnSelChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnListViewDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	LRESULT StartStopService(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	enum BuiltInIcon
	{
		icoGrey,
		icoYellow,
		icoGreen,
		icoRed,
		icoDriver,
		icoFsDriver,
		icoService,
		icoMultiService,
		icoInteractive,
	};
public:
	LRESULT OnBnClickedProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedAddservice(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedDeleteservice(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
