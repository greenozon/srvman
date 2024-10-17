#pragma once
#include "resource.h"
#include <bzswin/services.h>

#define BEGIN_MAP int get_something() {


class CPropertiesDlg : public CDialogImpl<CPropertiesDlg>, public CDialogResize<CPropertiesDlg>
{
private:
	BazisLib::Win32::Service *m_pService;
	bool m_bReadOnly;
	BazisLib::String m_Name;
	CImageList m_ImageList;
	CComboBoxEx m_cbServiceType;
	CComboBoxEx m_cbStartMode;
	bool m_bDoNotUpdatePaths;
	bool m_bNonServiceExe;

	//unsigned m_PathChangesToIgnore;

	enum ValueChangeFlags
	{
		vcfNone					= 0x00000000,
		vcfVisibleNameChanged	= 0x00000001,
		vcfPathChanged			= 0x00000002,
		vcfTypeChanged			= 0x00000004,
		vcfStartChanged			= 0x00000008,
		vcfGroupChanged			= 0x00000010,
		vcfLoginChanged			= 0x00000020,
		vcfDescChanged			= 0x00000040,
	} m_ValueChangeFlags;

	bool m_bLastChangedPathIsRawPath;


public:
	enum { IDD = IDD_SERVICEPROPS };

	BEGIN_MSG_MAP(CPropertiesDlg)
		COMMAND_HANDLER(IDC_BROWSE, BN_CLICKED, OnBnClickedBrowse)
		COMMAND_HANDLER(IDC_VISIBLENAME, EN_CHANGE, OnParamChanged)
		COMMAND_HANDLER(IDC_INTERACTIVE, BN_CLICKED, OnParamChanged)
		COMMAND_HANDLER(IDC_LOADGROUP, EN_CHANGE, OnParamChanged)
		COMMAND_HANDLER(IDC_LOGIN, EN_CHANGE, OnParamChanged)
		COMMAND_HANDLER(IDC_DESCRIPTION, EN_CHANGE, OnParamChanged)
		COMMAND_HANDLER(IDC_STARTMODE, CBN_SELCHANGE, OnParamChanged)
		CHAIN_MSG_MAP(CDialogResize)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnBnClickedCancel)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnBnClickedOk)
		COMMAND_HANDLER(IDC_PROPERTIES, BN_CLICKED, OnBnClickedProperties)
		COMMAND_HANDLER(IDC_SERVICETYPE, CBN_SELCHANGE, OnServiceTypeChanged)
		COMMAND_HANDLER(IDC_WIN32PATH, EN_CHANGE, OnWin32PathChanged);
		COMMAND_HANDLER(IDC_TRANSLATEDPATH, EN_CHANGE, OnRawPathChanged);
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CPropertiesDlg)
		DLGRESIZE_CONTROL(IDC_INTERNALNAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_VISIBLENAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_WIN32PATH, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_TRANSLATEDPATH, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_SERVICETYPE, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_STARTMODE, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_LOADGROUP, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_LOGIN, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_PASSWORD, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_PROPERTIES, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BROWSE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_INTERACTIVE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_DESCRIPTION, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_DESCRIPTION, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	CPropertiesDlg(const BazisLib::String &Name = _T(""), BazisLib::Win32::Service *pService = NULL, bool ReadOnly = false);

private:
	enum BuiltInIcon
	{
		icoDriver,
		icoFsDriver,
		icoService,
		icoMultiService,
		icoSysprogs,

		icoAuto,
		icoBoot,
		icoManual,
		icoDisabled,
		icoSystem,
	};

	enum ServiceType
	{
		stFsDriver,
		stKernelDriver,
		stOwnProcess,
		stSharedProcess,
		stNonServiceExe,
	};

	enum StartMode
	{
		smAuto,
		smBoot,
		smManual,
		smDisabled,
		smSystem,
	};

	void GetDlgItemTextToTypedBuffer(unsigned ID, BazisLib::TypedBuffer<TCHAR> *pBuf);
	unsigned GetNonServiceProcessCommandLine(LPCTSTR lpSrvManCommandLine);

public:
	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnBnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnServiceTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
	LRESULT OnWin32PathChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRawPathChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnBnClickedProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnParamChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
