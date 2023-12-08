#include <Windows.h>
#include <string>

int shutdownType;
UINT time;
bool force;
bool showMessage;
UINT secondsForMessage = 30;


DWORD SHUTDOWN_REASON = SHTDN_REASON_FLAG_PLANNED;
LPCWSTR LOGOFF_MESSAGE = L"ƒо запланированного выхода из системы";
LPCWSTR REBOOT_MESSAGE = L"ƒо запланированной перезагрузки";
LPCWSTR SHUTDOWN_MESSAGE = L"ƒо запланированного завершени€ работы";
const int MESSAGE_BUFFER_SIZE = 200;

VOID ConstructMessage(LPCWSTR startMessage, LPWSTR* message)
{
    std::wstring str = std::wstring(startMessage).append(std::wstring(L" осталось "))
        .append(std::to_wstring(secondsForMessage)).append(std::wstring(L" секунд."));
    wcsncpy_s(*message, MESSAGE_BUFFER_SIZE, str.c_str(), str.length());
}

VOID StartLoggingOff()
{
    UINT flags = EWX_LOGOFF;
    if (force)
    {
        flags |= EWX_FORCE;
    }
    if (showMessage)
    {
        LPWSTR message = new wchar_t[MESSAGE_BUFFER_SIZE];
        ConstructMessage(LOGOFF_MESSAGE, &message);
        InitiateSystemShutdownEx(NULL, message, secondsForMessage + 2, force, true, SHUTDOWN_REASON);
        Sleep(secondsForMessage * 1000);
        AbortSystemShutdown(NULL);
    }
    ExitWindowsEx(flags, SHUTDOWN_REASON);	
}

VOID StartShutdown()
{
    bool isReboot = shutdownType == 1;
    UINT flags = isReboot ? EWX_REBOOT : EWX_POWEROFF;
    if (force)
    {
        flags |= EWX_FORCE;
    }
    LPWSTR message = new wchar_t[MESSAGE_BUFFER_SIZE];
    ConstructMessage(isReboot ? REBOOT_MESSAGE : SHUTDOWN_MESSAGE, &message);
    InitiateSystemShutdownEx(NULL, message, secondsForMessage + 2, force, isReboot, SHUTDOWN_REASON);
    Sleep(secondsForMessage * 1000);
    AbortSystemShutdown(NULL);
}

VOID Shutdown(UINT flags, LPCWSTR msg)
{
    if (showMessage)
    {
        LPWSTR message = new wchar_t[MESSAGE_BUFFER_SIZE];
        ConstructMessage(msg, &message);
        InitiateSystemShutdownEx(NULL, message, secondsForMessage + 2, force, true, SHUTDOWN_REASON);
        Sleep(secondsForMessage * 1000);
        AbortSystemShutdown(NULL);
    }
    ExitWindowsEx(flags, SHUTDOWN_REASON);
}

VOID StartAction()
{
    UINT flags = 0U;
    LPCWSTR message = NULL;
    switch (shutdownType)
    {
    case 0:
        flags = EWX_POWEROFF;
        message = SHUTDOWN_MESSAGE;
        break;
    case 1:
        flags = EWX_REBOOT;
        message = REBOOT_MESSAGE;
        break;
    case 2:
        flags = EWX_LOGOFF;
        message = LOGOFF_MESSAGE;
        break;
    }
    if (force)
    {
        flags |= EWX_FORCE;
    }
    Shutdown(flags, message);
}

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    StartAction();
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

bool GetPrivileges()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp = {};
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;   
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
    return true;
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
    time = wcstoul(argv[2], NULL, 10);
    force = _wtoi(argv[3]) != 0;
    showMessage = _wtoi(argv[4]) != 0;

    GetPrivileges();

    const unsigned int defaultMessageTime = 30;
    if (showMessage)
    {
        secondsForMessage = time < defaultMessageTime ? time : defaultMessageTime;
    }
    else
    {
        secondsForMessage = 0;
    }
    if (time > secondsForMessage)
    {
        UINT_PTR timerId = SetTimer(NULL, 0, (time - secondsForMessage) * 1000, TimerProc);
    }
    else 
    {
        StartAction();
    }

    while (GetMessage(&msg, 0, 0, 0))
    {
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

