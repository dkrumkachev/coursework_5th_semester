#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <string>
#include <tlhelp32.h>
#pragma comment(lib, "comctl32.lib")

#define WINDOW_WIDTH 670
#define WINDOW_HEIGHT 400
#define BUTTON_ID 1
#define COMBOBOX_ID 2
#define MAX_DAYS

typedef struct ClockEdits {
	HWND Day;
	HWND Month;
	HWND Year;
	HWND Hour;
	HWND Minute;
} ClockEdits;

typedef struct TimerEdits {
	HWND Hours;
	HWND Minutes;
	HWND Seconds;
} TimerEdits;

ClockEdits clockEdits;
TimerEdits timerEdits;

HWND	hwndMainWindow;
HWND	hwndShutdownOptionCBB;
HWND	hwndTimeOptionCBB;
HWND	hwndForceShutdownCHB;
HWND	hwndShowMessageCHB;
HWND	hwndButton;
HWND	hwndLabel;

HWND*	clockControls = new HWND[15];
HWND*	timerControls = new HWND[9];

HFONT	hfDefaultFont = CreateFont(24, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

bool scheduled = false;

const wchar_t* CHILD_PROCESS_NAME = L"shutdown.exe";
STARTUPINFO si;
PROCESS_INFORMATION pi;
std::wstring plannedShutdownInfo = L"";
std::wstring PLANNED_SHUTDOWN_INFO_FILE_NAME = L"plannedshutdown.txt";



VOID WriteToFile(std::wstring fileName, std::wstring str) 
{
	LPCWSTR name = fileName.c_str();
	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwWritten;
	WriteFile(hFile, str.c_str(), sizeof(WCHAR) * (DWORD)str.length(), &dwWritten, NULL);
	CloseHandle(hFile);
}

std::wstring ReadFromFile(std::wstring fileName)
{
	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
	{	
		return std::wstring(L"");
	}
	
	wchar_t buffer[512] = {};
	DWORD dwRead = 0;
	if (ReadFile(hFile, buffer, sizeof(buffer), &dwRead, NULL) == 0)
	{
		return std::wstring(L"");
	}
	if (dwRead < 512)
	{
		buffer[dwRead] = '\0';
	}
	CloseHandle(hFile);
	return std::wstring(buffer);
}

HWND CreateLabel(LPCWSTR text, int& x, int y, HWND hwndParent)
{
	HDC hdc = GetDC(hwndParent);
	SelectObject(hdc, hfDefaultFont);
	SIZE size;
	GetTextExtentPoint32(hdc, text, lstrlenW(text), &size);
	ReleaseDC(hwndParent, hdc);
	HWND hwndLabel = CreateWindow(L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_SIMPLE,
		x, y, size.cx, 25, hwndParent, NULL, NULL, 0);
	SendMessage(hwndLabel, WM_SETFONT, (WPARAM)hfDefaultFont, (LPARAM)TRUE);
	x += size.cx;
	return hwndLabel;
}

HWND CreateComboBox(int& x, int y, HWND hwndParent, LPCWSTR values[], int count, int initial = 0)
{
	HDC hdc = GetDC(hwndParent);
	SelectObject(hdc, hfDefaultFont);
	SIZE size;
	int height = 30;
	int width = 0;
	for (int i = 0; i < count; i++)
	{
		GetTextExtentPoint32(hdc, values[i], lstrlenW(values[i]), &size);
		if (size.cx > width)
		{
			width = size.cx;
		}
		height += size.cy;
	}
	width += 30;
	ReleaseDC(hwndParent, hdc);
	HWND hwndComboBox = CreateWindow(L"COMBOBOX", L"", WS_CHILD | WS_BORDER | WS_VISIBLE | CBS_DROPDOWNLIST,
		x, y, width, height, hwndParent, NULL, NULL, 0);
	x += width;
	SendMessage(hwndComboBox, WM_SETFONT, (WPARAM)hfDefaultFont, (LPARAM)TRUE);
	for (int i = 0; i < count; i++)
	{
		SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)values[i]);
	}
	SendMessage(hwndComboBox, CB_SETCURSEL, initial, 0);
	return hwndComboBox;
}

HWND CreateTimeSetting(LPCWSTR timeUnits, int min, int max, int& x, int y,
	HWND hwndParent, HWND* controls, int& i, int initialValue = 0)
{
	int cy = 25;
	int cx = (int)(8 * std::to_string(max).length() + 30);
	HWND hwndEdit = CreateWindowEx(WS_EX_STATICEDGE, L"EDIT", L"",
		WS_CHILD | WS_VISIBLE | ES_NUMBER, x, y, cx, cy, hwndParent, NULL, NULL, 0);
	controls[i++] = hwndEdit;
	SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hfDefaultFont, (LPARAM)TRUE);
	x += cx;
	if (min == max) {
		SendMessage(hwndEdit, EM_SETREADONLY, (WPARAM)TRUE, 0);
	}
	else {
		DWORD style = WS_CHILD | WS_VISIBLE | UDS_AUTOBUDDY | UDS_WRAP | UDS_SETBUDDYINT | UDS_NOTHOUSANDS | UDS_ALIGNRIGHT;
		HWND hwndUpDown = CreateWindow(UPDOWN_CLASS, NULL, style, 0, 0, 0, 0, hwndParent, NULL, NULL, 0);
		controls[i++] = hwndUpDown;
		SendMessage(hwndUpDown, UDM_SETRANGE, 0, (LPARAM)MAKELONG(max, min));
	}
	SendMessage(hwndEdit, EM_SETLIMITTEXT, std::to_string(max).length(), 0);
	SetWindowText(hwndEdit, std::to_wstring(initialValue).c_str());
	x += 1;
	HWND hwndLabel = CreateLabel(timeUnits, x, y + 3, hwndParent);
	controls[i++] = hwndLabel;
	x += 1;
	return hwndEdit;
}

HWND CreateCheckBox(int& x, int y, HWND hwndParent)
{
	HWND checkBox = CreateWindow(L"BUTTON", NULL, BS_AUTOCHECKBOX | BS_FLAT | WS_CHILD | WS_VISIBLE,
		x, y, 25, 25, hwndParent, NULL, NULL, 0);
	return checkBox;
}

VOID CreateTimeSpecificControls(HWND hwndParent, int& x, int& y, int gap)
{
	int timeX = x + gap * 3;
	x = timeX;
	
	SYSTEMTIME time;
	GetLocalTime(&time);
	int maxYear = (time.wMonth == 12 && time.wDay > 1) ? time.wYear + 1 : time.wYear;

	int i = 0;
	clockEdits.Day = CreateTimeSetting(L".", 1, 31, x, y, hwndParent, clockControls, i, time.wDay);
	clockEdits.Month = CreateTimeSetting(L".", 1, 12, x, y, hwndParent, clockControls, i, time.wMonth);
	clockEdits.Year = CreateTimeSetting(L"", time.wYear, maxYear, x, y, hwndParent, clockControls, i, time.wYear);
	x += gap;
	clockEdits.Hour = CreateTimeSetting(L"ч", 0, 23, x, y, hwndParent, clockControls, i, time.wHour);
	clockEdits.Minute = CreateTimeSetting(L"мин", 0, 59, x, y, hwndParent, clockControls, i, time.wMinute);

	i = 0;
	x = timeX + gap - 7;
	timerEdits.Hours = CreateTimeSetting(L"ч", 0, 719, x, y, hwndParent, timerControls, i);
	timerEdits.Minutes = CreateTimeSetting(L"мин", 0, 59, x, y, hwndParent, timerControls, i);
	timerEdits.Seconds = CreateTimeSetting(L"с", 0, 59, x, y, hwndParent, timerControls, i);
	for (i = 0; i < 9; i++)
	{
		ShowWindow(timerControls[i], SW_HIDE);
	}
}

VOID CreateControls(HWND hwndParent)
{
	INITCOMMONCONTROLSEX icex = {};
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_UPDOWN_CLASS;
	BOOL r = InitCommonControlsEx(&icex);
	int gap = 14;
	int x = 140;
	int y = 30;
	int height = 25;

	CreateLabel(L"Выберите действие:", x, y, hwndParent);
	x += gap;
	LPCWSTR comboBoxOptions1[] = { L"Завершение работы", L"Перезагрузка", L"Выход из системы" };
	hwndShutdownOptionCBB = CreateComboBox(x, y - 3, hwndParent, comboBoxOptions1, 3);

	x = 40;
	y += 60;
	LPCWSTR comboBoxOptions2[] = { L"в указанное время", L"по истечении времени" };
	hwndTimeOptionCBB = CreateComboBox(x, y - 3, hwndParent, comboBoxOptions2, 2);

	CreateTimeSpecificControls(hwndParent, x, y, gap);

	x = 120;
	y += 60;
	CreateLabel(L"Принудительное завершение программ:", x, y, hwndParent);
	x += 45;
	int checkBoxStartX = x;
	hwndForceShutdownCHB = CreateCheckBox(x, y, hwndParent);
	x = 120;
	y += 60;
	CreateLabel(L"Уведомление перед завершением работы:", x, y, hwndParent);
	x = checkBoxStartX;
	hwndShowMessageCHB = CreateCheckBox(x, y, hwndParent);

	x = 80;
	y += 45;
	hwndLabel = CreateLabel(L"Завершение работы запланировано на 00.00.0000 00:00:00  ", x, y, hwndParent);
	ShowWindow(hwndLabel, SW_HIDE);
	
	x = 200;
	y += 40;
	hwndButton = CreateWindow(L"BUTTON", L"Запланировать", BS_PUSHBUTTON | BS_FLAT |
		WS_CHILD | WS_VISIBLE | WS_BORDER, x, y, 250, 50, hwndParent, (HMENU)BUTTON_ID, NULL, 0);
	SendMessage(hwndButton, WM_SETFONT, (WPARAM)hfDefaultFont, (LPARAM)TRUE);
}

BOOL StartShutdownProcess(int type, UINT time, bool force, bool showMessage)
{
	std::wstring cmd = std::wstring(CHILD_PROCESS_NAME)
		.append(std::wstring(L" ")).append(std::to_wstring(type))
		.append(std::wstring(L" ")).append(std::to_wstring(time))
		.append(std::wstring(L" ")).append(std::to_wstring(force ? 1 : 0))
		.append(std::wstring(L" ")).append(std::to_wstring(showMessage ? 1 : 0));
	LPWSTR commandLine = new wchar_t[cmd.length() + 1];
	wcsncpy_s(commandLine, cmd.length() + 1, cmd.c_str(), cmd.length() + 1);
	return CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

int GetValueFromComboBox(HWND hwndComboBox)
{
	return (int)SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0);
}

int GetValueFromEdit(HWND hwndEdit)
{
	WCHAR* value = new WCHAR[5];
	GetWindowText(hwndEdit, value, 5);
	return _wtoi(value);
}

bool GetValueFromCheckBox(HWND hwndCheckBox)
{
	return SendMessage(hwndCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

LONGLONG GetIntervalsLeftUntilDate(int day, int month, int year, int hour, int minute, ULARGE_INTEGER* current, ULARGE_INTEGER* input)
{
	SYSTEMTIME currentTime;
	SYSTEMTIME inputTime = {};
	inputTime.wYear = year;
	inputTime.wMonth = month;
	inputTime.wDay = day;
	inputTime.wHour = hour;
	inputTime.wMinute = minute;
	inputTime.wSecond = 0;
	inputTime.wMilliseconds = 0;
	GetLocalTime(&currentTime);
	FILETIME currentFileTime, inputFileTime;
	SystemTimeToFileTime(&currentTime, &currentFileTime);
	SystemTimeToFileTime(&inputTime, &inputFileTime);
	current->LowPart = currentFileTime.dwLowDateTime;
	current->HighPart = currentFileTime.dwHighDateTime;
	input->LowPart = inputFileTime.dwLowDateTime;
	input->HighPart = inputFileTime.dwHighDateTime;
	return input->QuadPart - (LONGLONG)current->QuadPart;
}

std::wstring TimeToString(SYSTEMTIME time, bool includeSeconds = false)
{
	std::wstring str(L"");
	if (time.wDay < 10)
	{
		str.append(std::wstring(L"0"));
	}
	str.append(std::to_wstring(time.wDay)).append(std::wstring(L"."));
	if (time.wMonth < 10)
	{
		str.append(std::wstring(L"0"));
	}
	str.append(std::to_wstring(time.wMonth)).append(std::wstring(L"."))
		.append(std::to_wstring(time.wYear)).append(std::wstring(L" "));
	if (time.wHour < 10)
	{
		str.append(std::wstring(L"0"));
	}
	str.append(std::to_wstring(time.wHour)).append(std::wstring(L":"));
	if (time.wMinute < 10)
	{
		str.append(std::wstring(L"0"));
	}
	str.append(std::to_wstring(time.wMinute));
	if (includeSeconds)
	{
		str.append(std::wstring(L":"));
		if (time.wSecond < 10)
		{
			str.append(std::wstring(L"0"));
		}
		str.append(std::to_wstring(time.wSecond));

	}
	return str;
}

std::wstring CheckForCorrectTime(int day, int month, int year, int hour, int minute)
{
	ULARGE_INTEGER currentUL = {}, inputUL = {};
	GetIntervalsLeftUntilDate(day, month, year, hour, minute, &currentUL, &inputUL);
	if (inputUL.QuadPart <= currentUL.QuadPart)
	{
		return L"Пожалуйста, укажите корректное время.";
	}
	const ULONGLONG intervalsIn30Days = 30LL * 24 * 60 * 60 * 10000000LL;
	ULONGLONG max = currentUL.QuadPart + intervalsIn30Days;
	if (inputUL.QuadPart > max)
	{
		ULARGE_INTEGER maxUL = {};
		maxUL.QuadPart = max;
		FILETIME maxFileTime = {};
		maxFileTime.dwLowDateTime = maxUL.LowPart;
		maxFileTime.dwHighDateTime = maxUL.HighPart;
		SYSTEMTIME maxTime = {};
		FileTimeToSystemTime(&maxFileTime, &maxTime);
		return std::wstring(L"Максимально допустимое время: ").append(TimeToString(maxTime));
	}
	return std::wstring();
}

UINT SecondsLeftUntilDate(int day, int month, int year, int hour, int minute)
{
	ULARGE_INTEGER currentTime = {}, inputTime = {};
	LONGLONG diff = GetIntervalsLeftUntilDate(day, month, year, hour, minute, &currentTime, &inputTime);
	return (UINT)(diff / 10000000);
}

VOID MakePlannedShutdownInfoString(UINT secondsLeft, int type)
{
	secondsLeft++;
	SYSTEMTIME currentTime;
	GetLocalTime(&currentTime);
	FILETIME currentFileTime;
	SystemTimeToFileTime(&currentTime, &currentFileTime);
	ULARGE_INTEGER currentUL = {};
	currentUL.LowPart = currentFileTime.dwLowDateTime;
	currentUL.HighPart = currentFileTime.dwHighDateTime;
	const ULONGLONG intervalsLeft = secondsLeft * 10000000ULL;
	currentUL.QuadPart += intervalsLeft;
	FILETIME shutdownFileTime = {};
	shutdownFileTime.dwLowDateTime = currentUL.LowPart;
	shutdownFileTime.dwHighDateTime = currentUL.HighPart;
	SYSTEMTIME time = {};
	FileTimeToSystemTime(&shutdownFileTime, &time);
	if (type == 0)
	{
		plannedShutdownInfo = std::wstring(L"Завершение работы запланировано на ");
	}
	else if (type == 1)
	{
		plannedShutdownInfo = std::wstring(L"Перезагрузка запланирована на ");
	}
	else if (type == 2)
	{
		plannedShutdownInfo = std::wstring(L"Выход из системы запланирован на ");
	}
	plannedShutdownInfo.append(TimeToString(time, true));
}

bool GetTimeInSecondsFromControls(UINT* timeInSeconds)
{
	int timeOption = GetValueFromComboBox(hwndTimeOptionCBB);
	if (timeOption == 0)
	{
		int hour = GetValueFromEdit(clockEdits.Hour);
		int minute = GetValueFromEdit(clockEdits.Minute);
		int day = GetValueFromEdit(clockEdits.Day);
		int month = GetValueFromEdit(clockEdits.Month);
		int year = GetValueFromEdit(clockEdits.Year);
		std::wstring errorMessage = CheckForCorrectTime(day, month, year, hour, minute);
		if (errorMessage.length() != 0)
		{
			MessageBox(NULL, errorMessage.c_str(), L"Некорректное значение времени", MB_OK);
			return false;
		}
		*timeInSeconds = SecondsLeftUntilDate(day, month, year, hour, minute);
	}
	else
	{
		int hours = GetValueFromEdit(timerEdits.Hours);
		int minutes = GetValueFromEdit(timerEdits.Minutes);
		int seconds = GetValueFromEdit(timerEdits.Seconds);
		*timeInSeconds = (hours * 60 + minutes) * 60 + seconds;
	}
	return true;
}

bool Schedule()
{
	int type = GetValueFromComboBox(hwndShutdownOptionCBB);
	bool forceQuit = GetValueFromCheckBox(hwndForceShutdownCHB);
	bool showMessage = GetValueFromCheckBox(hwndShowMessageCHB);
	UINT timeInSeconds;
	if (!GetTimeInSecondsFromControls(&timeInSeconds))
	{
		return false;
	}
	if (!StartShutdownProcess(type, timeInSeconds, forceQuit, showMessage))
	{
		MessageBox(NULL, L"Ошибка при планировании завершения работы.", L"Ошибка", MB_OK);
		return false;
	}
	MakePlannedShutdownInfoString(timeInSeconds, type);
	WriteToFile(PLANNED_SHUTDOWN_INFO_FILE_NAME, plannedShutdownInfo);
	SetWindowText(hwndLabel, plannedShutdownInfo.c_str());
	ShowWindow(hwndLabel, SW_SHOW);
	SetWindowText(hwndButton, L"Отменить");
	return true;
}

VOID Cancel()
{
	TerminateProcess(pi.hProcess, 0);
	AbortSystemShutdown(NULL);
	WriteToFile(PLANNED_SHUTDOWN_INFO_FILE_NAME, L"");
	ShowWindow(hwndLabel, SW_HIDE);
	SetWindowText(hwndButton, L"Запланировать");
	scheduled = false;
}

VOID ButtonClicked()
{
	if (scheduled)
	{
		Cancel();
	}
	else if (Schedule())
	{
		scheduled = true;
	}
}


VOID ChangeTimeInput()
{
	LRESULT selected = SendMessage(hwndTimeOptionCBB, CB_GETCURSEL, 0, 0);
	for (int i = 0; i < 15; i++)
	{
		ShowWindow(clockControls[i], selected == 0 ? SW_SHOW : SW_HIDE);
	}
	for (int i = 0; i < 9; i++)
	{
		ShowWindow(timerControls[i], selected == 0 ? SW_HIDE : SW_SHOW);
	}
}


LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	switch (uMsg) {
	case WM_CREATE:
		CreateControls(hWnd);
		if (pi.hProcess != NULL)
		{
			plannedShutdownInfo = ReadFromFile(PLANNED_SHUTDOWN_INFO_FILE_NAME);
			SetWindowText(hwndLabel, plannedShutdownInfo.c_str());
			ShowWindow(hwndLabel, SW_SHOW);
			SetWindowText(hwndButton, L"Отменить");
			scheduled = true;
		}
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE && lParam == (LPARAM)hwndTimeOptionCBB)
		{
			ChangeTimeInput();
		}
		if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == BUTTON_ID)
		{
			ButtonClicked();
		}
		break;
	case WM_CTLCOLORSTATIC:
		if (lParam == (LPARAM)hwndLabel)
		{
			hdc = (HDC)wParam;
			SetBkColor(hdc, RGB(0, 255, 0));
			return (LRESULT)CreateSolidBrush(RGB(255, 255, 255));
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

VOID EnableShutdownPrivileges()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp = {};
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
}

VOID EnableDebugPriveleges() 
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp = {};

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, false, &tkp, sizeof(tkp), NULL, NULL);
	CloseHandle(hToken);
}

HANDLE GetShutdownProcessHandle() 
{
	EnableDebugPriveleges();
	PROCESSENTRY32 entry = {};
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(snapshot, &entry) == TRUE) 
	{
		while (Process32Next(snapshot, &entry) == TRUE) 
		{
			std::wstring exeFile = std::wstring(entry.szExeFile);
			if (exeFile._Equal(CHILD_PROCESS_NAME)) 
			{
				return OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
			}
		}
	}
	return NULL;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	pi.hProcess = GetShutdownProcessHandle();

	EnableShutdownPrivileges();
	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_GLOBALCLASS;
	wcex.lpfnWndProc = MainWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(0, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"MainWindowClassName";
	wcex.hIconSm = NULL;
	RegisterClassEx(&wcex);
	hwndMainWindow = CreateWindowEx(0, L"MainWindowClassName", L"Shutdown scheduler",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 0, NULL);

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return (int)msg.wParam;
}