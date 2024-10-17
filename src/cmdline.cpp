#include "stdafx.h"
#include "cmdline.h"
#include <bzs_string.h>
#include <Win32/services.h>
#include <file.h>

#include <map>
#include <vector>

static const char szCommandLineHelpMsg[] = "Command-line options:\r\n\
srvman add <binary file> [service name] [display name]\r\n\
           [/type:drv/exe/fsd]\r\n\
           [/start:boot/sys/man/dis]\r\n\
		   [/interactive:no]\r\n\
		   [/overwrite:yes]\r\n\
srvman drvadd <driver service name>\r\n\
srvman delete <service name>\r\n\
srvman start <service name> [/nowait] [/delay:msec]\r\n\
srvman stop <service name> [/nowait] [/delay:msec]\r\n\
srvman restart <service name> [/nowait] [/delay:msec]\r\n\
srvman run <.sys file> [/copy:yes] [/overwrite:no] [/stopafter:<msec>]\r\n\
srvman /? - Display this message\n\r\n\
See http://tools.sysprogs.org/srvman/cmdline.shtml for details\r\n";
const char szPressAnyKey[] = "Press any key to continue...";


static int ConsolePrintA(HANDLE hStdOut, const char *pStr)
{
	DWORD dwDone = 0;
	WriteFile(hStdOut, pStr, (DWORD)strlen(pStr), &dwDone, NULL);
	return dwDone;
}

using namespace BazisLib;
using namespace BazisLib::Win32;

class CommandLineArgContainer
{
private:
	std::map<iwstring, int> intSwitches;
	std::map<iwstring, iwstring> strSwitches;
	std::vector<iwstring> params;

public:
	CommandLineArgContainer(int argc, LPWSTR *pArgv)
	{
		for (int i = 1; i < argc; i++)
		{
			if (pArgv[i][0] != '/')
				params.push_back(pArgv[i]);
			else
			{
				wchar_t *p = wcschr(pArgv[i], '=');
				if (!p)
					p = wcschr(pArgv[i], ':');
				if (!p)
					intSwitches[pArgv[i] + 1] = 1, strSwitches[pArgv[i] + 1] = _T("");
				else
				{
					iwstring swName(pArgv[i] + 1, (p - pArgv[i] - 1));
					unsigned val = 0;
					if (!_wcsicmp(p + 1, L"on"))
						val = 1;
					else if (!_wcsicmp(p + 1, L"yes"))
						val = 1;
					else if (!_wcsicmp(p + 1, L"true"))
						val = 1;
					else
						val = _wtoi(p + 1);
					intSwitches[swName] = val;
					strSwitches[swName] = p + 1;
				}
			}
		}
	}

	bool SwitchExists(const iwstring &name)
	{
		return intSwitches.find(name) != intSwitches.end();
	}

	int GetIntSwitch(const iwstring &name, int defValue = 0)
	{
		if (!SwitchExists(name))
			return defValue;
		return intSwitches[name];
	}

	bool GetBoolSwitch(const iwstring &name, bool defValue = false)
	{
		if (!SwitchExists(name))
			return defValue;
		return (intSwitches[name] != 0);
	}

	iwstring GetStrSwitch(const iwstring &name, const iwstring &defValue = _T(""))
	{
		if (!SwitchExists(name))
			return defValue;
		return strSwitches[name];
	}

	const wchar_t *GetStrSwitchPtr(const iwstring &name, const wchar_t *pDefValue = NULL)
	{
		if (!SwitchExists(name))
			return pDefValue;
		return strSwitches[name].c_str();
	}

	const std::vector<iwstring> Params()
	{
		return params;
	}

	const wchar_t *GetStrParamPtr(unsigned Index)
	{
		if (Index >= params.size())
			return NULL;
		return params[Index].c_str();
	}

};

#define SERVICE_WIN32_APP_AS_SERVICE	0x55FFAAFF

String BuildCommandLineForAppAsService(LPCTSTR lpOriginalCmdLine, LPCTSTR lpServiceName);


ActionStatus CmdlInstallService(HANDLE hStdOut, LPCTSTR lpBinPath, LPCTSTR lpSvcName, LPCTSTR lpDisplayName, DWORD dwType, DWORD dwStart, bool Overwrite)
{
	ServiceControlManager mgr(SC_MANAGER_CREATE_SERVICE);
	if (!lpBinPath)
		return MAKE_STATUS(InvalidParameter);

	String strTmp;
	if (!lpSvcName)
	{
		int argc = 0;
		LPWSTR *pArgv = CommandLineToArgvW(lpBinPath, &argc);
		if (pArgv && pArgv[0])
			strTmp = FilePath(pArgv[0]).GetFileNameBase();
		else
			strTmp = FilePath(lpBinPath).GetFileNameBase();
		if (pArgv)
			LocalFree(pArgv);
		
		lpSvcName = strTmp.c_str();
	}

	String strTmp2;
	if (dwType == SERVICE_WIN32_APP_AS_SERVICE)
	{
		strTmp2 = BuildCommandLineForAppAsService(lpBinPath, lpSvcName);
		lpBinPath = strTmp2.c_str();
		dwType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
	}
	else if (dwType & SERVICE_DRIVER)
	{
		strTmp2 = Service::Win32PathToDriverPath(lpBinPath, false, true);
		lpBinPath = strTmp2.c_str();
	}

	ActionStatus st = mgr.CreateService(NULL, lpSvcName, lpDisplayName, SERVICE_ALL_ACCESS, dwType, dwStart, SERVICE_ERROR_NORMAL, lpBinPath);
	if ((st.GetErrorCode() == HRESULT_FROM_WIN32(ERROR_SERVICE_EXISTS)) && Overwrite)
	{
		Service svc(&mgr, lpSvcName, SERVICE_CHANGE_CONFIG);
		st = svc.ChangeConfig(dwType, dwStart, SERVICE_NO_CHANGE, lpBinPath);
	}
	return st;
}

static DWORD SrvTypeFromString(const TCHAR *pStr, DWORD dwDefType, bool Interactive)
{
	if (!pStr)
		return dwDefType;
	DWORD dwInteractiveBit = Interactive ? SERVICE_INTERACTIVE_PROCESS : 0;
	if (!_tcsnicmp(pStr, _T("drv"), _tcslen(pStr)))
		return SERVICE_KERNEL_DRIVER;
	else if (!_tcsnicmp(pStr, _T("driver"), _tcslen(pStr)))
		return SERVICE_KERNEL_DRIVER;
	else if (!_tcsnicmp(pStr, _T("fsdriver"), _tcslen(pStr)))
		return SERVICE_FILE_SYSTEM_DRIVER;
	else if (!_tcsnicmp(pStr, _T("executable"), _tcslen(pStr)))
		return SERVICE_WIN32_OWN_PROCESS | dwInteractiveBit;
	else if (!_tcsnicmp(pStr, _T("sharedexecutable"), _tcslen(pStr)))
		return SERVICE_WIN32_SHARE_PROCESS | dwInteractiveBit;
	else if (!_tcsnicmp(pStr, _T("application"), _tcslen(pStr)))
		return SERVICE_WIN32_APP_AS_SERVICE;
	return dwDefType;
}

static DWORD SrvStartFromString(const TCHAR *pStr, DWORD dwDefStart = SERVICE_DEMAND_START)
{
	if (!pStr)
		return dwDefStart;
	if (!_tcsnicmp(pStr, _T("boot"), _tcslen(pStr)))
		return SERVICE_BOOT_START;
	if (!_tcsnicmp(pStr, _T("system"), _tcslen(pStr)))
		return SERVICE_SYSTEM_START;
	if (!_tcsnicmp(pStr, _T("automatic"), _tcslen(pStr)))
		return SERVICE_AUTO_START;
	if (!_tcsnicmp(pStr, _T("manual"), _tcslen(pStr)))
		return SERVICE_DEMAND_START;
	if (!_tcsnicmp(pStr, _T("disabled"), _tcslen(pStr)))
		return SERVICE_DISABLED;
	return dwDefStart;	
}

ActionStatus CmdlDeleteService(HANDLE hStdOut, LPCTSTR lpSvcName)
{
	if (!lpSvcName)
		return MAKE_STATUS(InvalidParameter);
	ServiceControlManager mgr(SC_MANAGER_ENUMERATE_SERVICE);
	Service svc(&mgr, lpSvcName, DELETE);
	return svc.Delete();
}

static void WaitIfNeeded(HANDLE hStdOut, unsigned Delay)
{
	if (Delay)
	{
		char sz[128];
		_snprintf_s(sz, sizeof(sz), "Waiting for %d milliseconds...\n", Delay);
		ConsolePrintA(hStdOut, sz);
		Sleep(Delay);
	}
}

ActionStatus CmdlControlService(HANDLE hStdOut, LPCTSTR lpSvcName, DWORD dwCtlCode, DWORD dwAccess, unsigned Delay, bool NoWait)
{
	if (!lpSvcName)
		return MAKE_STATUS(InvalidParameter);
	ServiceControlManager mgr(SC_MANAGER_ENUMERATE_SERVICE);
	Service svc(&mgr, lpSvcName, dwAccess | SERVICE_QUERY_STATUS);
	ActionStatus st = svc.GetOpenStatus();
	if (!st.Successful())
		return st;
	WaitIfNeeded(hStdOut, Delay);

	st = svc.Control(dwCtlCode);
	if (!st.Successful())
		return st;
	if (!NoWait)
	{
		if (dwCtlCode == SERVICE_CONTROL_STOP)
		{
			while (svc.QueryState() == SERVICE_STOP_PENDING)
			{
				Sleep(100);
			}
		}
	}
	return MAKE_STATUS(Success);
}

ActionStatus CmdlStartService(HANDLE hStdOut, LPCTSTR lpSvcName, unsigned Delay, bool NoWait)
{
	if (!lpSvcName)
		return MAKE_STATUS(InvalidParameter);
	ServiceControlManager mgr(SC_MANAGER_ENUMERATE_SERVICE);
	Service svc(&mgr, lpSvcName, SERVICE_START | SERVICE_QUERY_STATUS);
	ActionStatus st = svc.GetOpenStatus();
	if (!st.Successful())
		return st;
	WaitIfNeeded(hStdOut, Delay);

	st = svc.Start();
	if (!st.Successful())
		return st;
	if (!NoWait)
	{
		while (svc.QueryState() == SERVICE_START_PENDING)
		{
			Sleep(100);
		}
	}
	return MAKE_STATUS(Success);
}

ActionStatus CmdlRestartService(HANDLE hStdOut, LPCTSTR lpSvcName, unsigned Delay, bool NoWait)
{
	ActionStatus st = CmdlControlService(hStdOut, lpSvcName, SERVICE_CONTROL_STOP, SERVICE_STOP, Delay, false);
	if (!st.Successful() && (st.GetErrorCode() != HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE)))
		return st;
	return CmdlStartService(hStdOut, lpSvcName, 0, NoWait);
}

ActionStatus CmdlRunBinary(HANDLE hStdOut, LPCTSTR lpBinary, LPCTSTR lpSvcName, bool Copy, bool Overwrite, unsigned StopAfter)
{
	if (!lpBinary)
		return MAKE_STATUS(InvalidParameter);
	LPCTSTR lpTargetBinary = lpBinary;
	FilePath fp(_T(""));
	String strTmp;

	if (Copy || !lpSvcName)
	{
		int argc = 0;
		FilePath fpSrc(_T(""));
		LPWSTR *pArgv = CommandLineToArgvW(lpBinary, &argc);
		if (pArgv && pArgv[0])
			fpSrc = FilePath(pArgv[0]);
		else
			fpSrc = FilePath(lpBinary);
		if (pArgv)
			LocalFree(pArgv);

		if (!lpSvcName)
		{
			strTmp = fpSrc.GetFileNameBase();
			lpSvcName = strTmp.c_str();
		}

		if (Copy)
		{
			TCHAR tsz[MAX_PATH];
			GetSystemDirectory(tsz, __countof(tsz));
			fp = tsz;
			fp.AppendPath(_T("drivers"));
			fp.AppendPath(fpSrc.GetFileName().c_str());
			if (BazisLib::File::Exists(fp.c_str()) && !Overwrite)
				return MAKE_STATUS(AlreadyExists);
			if (!CopyFile(fpSrc.c_str(), fp.c_str(), !Overwrite))
				return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
			lpTargetBinary = fp.c_str();
		}
	}

	ActionStatus st = CmdlInstallService(hStdOut, lpTargetBinary, lpSvcName, NULL, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, Overwrite);
	if (!st.Successful())
		return st;
	st = CmdlStartService(hStdOut, lpSvcName, 0, false);
	if (!st.Successful())
		return st;
	if (StopAfter == -1)
		return MAKE_STATUS(Success);

	WaitIfNeeded(hStdOut, StopAfter);
	return CmdlControlService(hStdOut, lpSvcName, SERVICE_CONTROL_STOP, SERVICE_STOP, 0, false);
}

int ConsoleMain( int argc, LPWSTR *pArgv, bool ConsoleExisted )
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CommandLineArgContainer cmdArgs(argc, pArgv);
	
	ActionStatus st;
	bool Parsed = false;

	if (cmdArgs.Params().size() >= 2)
	{
		Parsed = true;

		if (cmdArgs.Params()[0] == L"add")
		{
			FilePath fp(cmdArgs.Params()[1].c_str());
			DWORD dwDefType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
			if (!_tcsnicmp(fp.GetExtension().c_str(), _T("sys"), 3))
				dwDefType = SERVICE_KERNEL_DRIVER;

			fp.ConvertToAbsolute();

			st = CmdlInstallService(hStdOut,
									 fp.c_str(),
									 cmdArgs.GetStrParamPtr(2),
									 cmdArgs.GetStrParamPtr(3),
									 SrvTypeFromString(cmdArgs.GetStrSwitchPtr(L"type"), dwDefType, cmdArgs.GetBoolSwitch(L"interactive", false)),
									 SrvStartFromString(cmdArgs.GetStrSwitchPtr(L"start")),
									 cmdArgs.GetBoolSwitch(L"overwrite", false));
		}
		else if (cmdArgs.Params()[0] == L"drvadd")
		{
			FilePath fp = FilePath::GetSpecialDirectoryLocation(dirDrivers);
			fp.AppendAndReturn(cmdArgs.GetStrParamPtr(1)).append(_T(".sys"));
			st = CmdlInstallService(hStdOut,
				fp.c_str(),
				cmdArgs.GetStrParamPtr(1),
				cmdArgs.GetStrParamPtr(2),
				SrvTypeFromString(cmdArgs.GetStrSwitchPtr(L"type"), SERVICE_KERNEL_DRIVER, cmdArgs.GetBoolSwitch(L"interactive", false)),
				SrvStartFromString(cmdArgs.GetStrSwitchPtr(L"start")),
				cmdArgs.GetBoolSwitch(L"overwrite", false));
		}
		else if (cmdArgs.Params()[0] == L"delete")
			st = CmdlDeleteService(hStdOut, cmdArgs.GetStrParamPtr(1));
		else if (cmdArgs.Params()[0] == L"start")
			st = CmdlStartService(hStdOut, cmdArgs.GetStrParamPtr(1), cmdArgs.GetIntSwitch(L"delay"), cmdArgs.GetBoolSwitch(L"nowait", false));
		else if (cmdArgs.Params()[0] == L"stop")
			st = CmdlControlService(hStdOut, cmdArgs.GetStrParamPtr(1), SERVICE_CONTROL_STOP, SERVICE_STOP, cmdArgs.GetIntSwitch(L"delay"), cmdArgs.GetBoolSwitch(L"nowait", false));
		else if (cmdArgs.Params()[0] == L"restart")
			st = CmdlRestartService(hStdOut, cmdArgs.GetStrParamPtr(1), cmdArgs.GetIntSwitch(L"delay"), cmdArgs.GetBoolSwitch(L"nowait", false));
		else if (cmdArgs.Params()[0] == L"run")
			st = CmdlRunBinary(hStdOut, cmdArgs.GetStrParamPtr(1), cmdArgs.GetStrParamPtr(2), cmdArgs.GetBoolSwitch(L"copy", false), cmdArgs.GetBoolSwitch(L"overwrite", false), cmdArgs.GetIntSwitch(L"stopafter", -1));
		else
			Parsed = false;
	}

	if (Parsed)
	{
		std::string strResult = StringToANSIString(st.GetMostInformativeText()).c_str();
		ConsolePrintA(hStdOut, strResult.c_str());
	}
	else
		ConsolePrintA(hStdOut, szCommandLineHelpMsg);

	bool DoPause = !ConsoleExisted;
	if (cmdArgs.SwitchExists(L"pause"))
		DoPause = cmdArgs.GetBoolSwitch(L"pause");

	if (DoPause)
	{
		DWORD dwDone;
		char ch = 0;
		ConsolePrintA(hStdOut, szPressAnyKey);
		ReadFile(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &dwDone, NULL);
	}
	return 0;
}
