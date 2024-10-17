#include "stdafx.h"
#include "runassrv.h"
#include <bzswin/services.h>

using namespace BazisLib::Win32;

class CommandLineRunnerService : public OwnProcessServiceWithWorkerThread
{
public:
	String m_CommandLine;

private:
	static BOOL CALLBACK WindowCloserProc(HWND hWnd, LPARAM lParam)
	{
		if ((GetWindowThreadProcessId(hWnd, NULL) == lParam) && !(GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD))
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		return TRUE;
	}

public:
	CommandLineRunnerService(LPCTSTR lpCommandLine, LPCTSTR lpServiceName)
		: OwnProcessServiceWithWorkerThread(lpServiceName, true, false)
		, m_CommandLine(lpCommandLine)
	{
	}

	virtual ActionStatus WorkerThreadBody(HANDLE hStopEvent) override
	{
		PROCESS_INFORMATION PI = {0,};
		STARTUPINFO SI = {sizeof(STARTUPINFO), };
		SI.wShowWindow = SW_SHOW;
		if (!CreateProcess(NULL, (LPWSTR)m_CommandLine.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &SI, &PI))
			return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
		ReportState(SERVICE_RUNNING);
		HANDLE hhWait[2] = {PI.hProcess, hStopEvent};
		if (WaitForMultipleObjects(__countof(hhWait), hhWait, FALSE, INFINITE) != WAIT_OBJECT_0)
		{
			EnumWindows(WindowCloserProc, PI.dwThreadId);
			if (WaitForSingleObject(PI.hProcess, 2000) != WAIT_OBJECT_0)
				TerminateProcess(PI.hProcess, -1);
		}
		return MAKE_STATUS(Success);
	}

};

int RunCommandLineAsService( LPCTSTR lpCommandLineToRun, LPCTSTR lpServiceName )
{
	return CommandLineRunnerService(lpCommandLineToRun, lpServiceName).EntryPoint();
}