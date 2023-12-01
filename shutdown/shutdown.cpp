#include <Windows.h>

int shutdownType;

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	UINT flags = 0;
	switch (shutdownType)
	{
	case 0:
		flags = EWX_SHUTDOWN;
		break;
	case 1:
		flags = EWX_REBOOT;
		break;
	default:
		flags = EWX_LOGOFF;
	}
	int a;
	if (ExitWindowsEx(flags /*| EWX_FORCE*/, SHTDN_REASON_FLAG_PLANNED) == 0)
		a = GetLastError();
	PostQuitMessage(0);
}

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	WNDCLASSEX wcex;
	MSG msg;

	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = MainWindowProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = L"MainWindowClassName";
	RegisterClassEx(&wcex);
	CreateWindow(L"MainWindowClassName", NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, 0);
	
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
	shutdownType = _wtoi(argv[1]);
	int hours = _wtoi(argv[2]);
	int minutes = _wtoi(argv[3]);
	int seconds = _wtoi(argv[4]);
	UINT time = ((hours * 60 + minutes) * 60 + seconds) * 1000;
	UINT_PTR timerId = SetTimer(NULL, 0, time, TimerProc);
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	// Get a token for this process. 
	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return(FALSE);

	// Get the LUID for the shutdown privilege. 
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
		&tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Get the shutdown privilege for this process. 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
		(PTOKEN_PRIVILEGES)NULL, 0);

	while (GetMessage(&msg, 0, 0, 0))
	{
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

