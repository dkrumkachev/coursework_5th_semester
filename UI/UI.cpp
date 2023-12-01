#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <string>
#pragma comment(lib, "comctl32.lib")

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 500
#define BUTTON_ID 1
#define COMBOBOX_ID 2

HWND	hwndMainWindow;
HWND	hwndShutdownOptionCBB;
HWND	hwndTimeOptionCBB;
HWND	hwndButton;
HWND	hwndForceShutdownCHB;
HWND*	clockControls = new HWND[15];
HWND*	timerControls = new HWND[9];

HFONT	hfDefaultFont = CreateFont(24, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
	ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");


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
	width += 25;
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
	int cx = 8 * std::to_string(max).length() + 30;
	HWND hwndEdit = CreateWindowEx(WS_EX_STATICEDGE, L"EDIT", L"",
		WS_CHILD | WS_VISIBLE | ES_NUMBER, x, y, cx, cy, hwndParent, NULL, NULL, 0);
	SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hfDefaultFont, (LPARAM)TRUE);
	x += cx;
	DWORD style = WS_CHILD | WS_VISIBLE | UDS_AUTOBUDDY | UDS_WRAP | UDS_SETBUDDYINT | UDS_NOTHOUSANDS | UDS_ALIGNRIGHT;
	HWND hwndUpDown = CreateWindow(UPDOWN_CLASS, NULL, style, 0, 0, 0, 0, hwndParent, NULL, NULL, 0);
	SendMessage(hwndUpDown, UDM_SETRANGE, 0, MAKELPARAM(max, 0));
	SendMessage(hwndEdit, EM_SETLIMITTEXT, std::to_string(max).length(), 0);
	SetWindowText(hwndEdit, std::to_wstring(initialValue).c_str());
	x += 1;
	HWND hwndLabel = CreateLabel(timeUnits, x, y + 3, hwndParent);
	x += 1;
	controls[i++] = hwndEdit;
	controls[i++] = hwndUpDown;
	controls[i++] = hwndLabel;
	return hwndEdit;
}

HWND CreateCheckBox(int& x, int y, HWND hwndParent)
{
	HWND checkBox = CreateWindow(L"BUTTON", NULL, BS_AUTOCHECKBOX | BS_FLAT | WS_CHILD | WS_VISIBLE,
		x, y, 25, 25, hwndParent, NULL, NULL, 0);
	return checkBox;
}

VOID CreateControls(HWND hwndParent)
{
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_UPDOWN_CLASS;
	BOOL r = InitCommonControlsEx(&icex);
	int gap = 14;
	int x = 40;
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

	int timeX = x + gap * 2;
	x = timeX;
	SYSTEMTIME time;
	GetLocalTime(&time);
	int i = 0;
	CreateTimeSetting(L".", 1, 31, x, y, hwndParent, clockControls, i, time.wDay);
	CreateTimeSetting(L".", 1, 12, x, y, hwndParent, clockControls, i, time.wMonth);
	CreateTimeSetting(L"", 2023, 2999, x, y, hwndParent, clockControls, i, time.wYear);
	x += gap * 2;
	CreateTimeSetting(L"ч", 0, 23, x, y, hwndParent, clockControls, i, time.wHour);
	CreateTimeSetting(L"мин", 0, 59, x, y, hwndParent, clockControls, i, time.wMinute);

	i = 0;
	x = timeX;
	CreateTimeSetting(L"ч", 0, 999, x, y, hwndParent, timerControls, i);
	CreateTimeSetting(L"мин", 0, 59, x, y, hwndParent, timerControls, i);
	CreateTimeSetting(L"с", 0, 59, x, y, hwndParent, timerControls, i);
	for (i = 0; i < 9; i++)
	{
		ShowWindow(timerControls[i], SW_HIDE);
	}

	x = 40;
	y += 60;
	CreateLabel(L"Принудительно завершить запущенные процессы:", x, y, hwndParent);
	x += 5;
	hwndForceShutdownCHB = CreateCheckBox(x, y, hwndParent);

	x = 200;
	y += 100;
	HWND hwndButton = CreateWindow(L"BUTTON", L"Запланировать", BS_PUSHBUTTON | BS_FLAT |
		WS_CHILD | WS_VISIBLE | WS_BORDER, x, y, 300, 50, hwndParent, (HMENU)BUTTON_ID, NULL, 0);
	SendMessage(hwndButton, WM_SETFONT, (WPARAM)hfDefaultFont, (LPARAM)TRUE);
}

BOOL StartShutdownProcess(int type, int hours, int minutes, int seconds)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	LPWSTR commandLine = new wchar_t[22];
	wcsncpy_s(commandLine, 22, L"shutdown.exe 1 0 0 10", 22);
	return CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi));
}

int GetValueFromComboBox(HWND hwndComboBox)
{
	return SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0);
}

int GetValueFromEdit(HWND hwndEdit)
{
	SendMessage(hwndEdit, WM_GETTEXT, )
}

VOID ButtonClicked()
{
	int type = GetValueFromComboBox(hwndShutdownOptionCBB);
	
	if (!StartShutdownProcess())
	{
		MessageBox(NULL, L"Error", L"Error", MB_OK);
	}
}


VOID ChangeTimeInput()
{
	int selected = SendMessage(hwndTimeOptionCBB, CB_GETCURSEL, 0, 0);
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
	switch (uMsg) {
	case WM_CREATE:
		CreateControls(hWnd);
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

	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}