// GGXrdChangeBorderlessWindowPos.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GGXrdChangeBorderlessWindowPos.h"
#include <commdlg.h>
#include <commctrl.h>
#include <vector>
#include <sstream>
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")
#include "const_obfuscate.h"
#include "Psapi.h"
#include "WError.h"
#include "Sigscanning.h"
#include "Version.h"

#define MAX_LOADSTRING 100

#undef OBF_FUNC
#define OBF_FUNC(name) name##Ptr
#define OBF_FUNC_DOUBLE(name) OBF_FUNC(name)

#define OBF_LOAD(module) \
	if (!module) { \
		module = LoadLibraryA(OBF_DATA(#module)); \
	} \
	if (!module) ExitProcess(1);

#undef OBF_IMPORT
#define OBF_IMPORT(module, func) OBF_FUNC(func) = reinterpret_cast<decltype(&func)>(GetProcAddress(module, OBF_DATA(#func)));
#define OBF_IMPORT_WITH_NAME(module, func, funcName) OBF_FUNC(func) = reinterpret_cast<decltype(&func)>(GetProcAddress(module, OBF_DATA(funcName)));

#define OBF_VALUE(type, name, val) \
	volatile const type name##ar[1] { val }; \
	type name = *const_obfuscate::deobfuscate(const_obfuscate::deobfuscate(name##ar, __LINE__).data, __LINE__).data;

HMODULE kernel32 = NULL;
HMODULE user32 = NULL;
HMODULE Psapi = NULL;
OBF_IMPORT_DECL(FindWindowW);
OBF_IMPORT_DECL(GetWindowThreadProcessId);
OBF_IMPORT_DECL(OpenProcess);
OBF_IMPORT_DECL(EnumProcessModulesEx);
OBF_IMPORT_DECL(GetModuleInformation);
OBF_IMPORT_DECL(WriteProcessMemory);

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HFONT font = NULL;
HWND mainWindow = NULL;
HWND DeviceNameTitleHwnd = NULL;
HWND DeviceStringTitleHwnd = NULL;
HWND MonitorNameTitleHwnd = NULL;
HWND MonitorStringTitleHwnd = NULL;
HWND RectTitleHwnd = NULL;
HWND WorkRectTitleHwnd = NULL;
std::vector<HWND> whiteBgLabels;
HBRUSH hbrBkgnd = NULL;
GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR           gdiplusToken;
wchar_t snatchedXrdPath[MAX_PATH] { L'\0' };
BYTE* fileBase = nullptr;
#if defined( _WIN64 )
typedef PIMAGE_NT_HEADERS32 nthdr;
#undef IMAGE_FIRST_SECTION
#define IMAGE_FIRST_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)        \
    ((ULONG_PTR)(ntheader) +                                            \
     FIELD_OFFSET( IMAGE_NT_HEADERS32, OptionalHeader ) +                 \
     ((ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))
#else
typedef PIMAGE_NT_HEADERS nthdr;
#endif
nthdr pNtHeader = nullptr;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
DWORD findOpenGgProcess(HWND* outHwnd);
const char* const OUR_SECTION_NAME = ".movbdls";

wchar_t wstrbuf[1024];

struct MonitorElement {
	DISPLAY_DEVICEW device;
	HWND deviceNameHwnd = NULL;
	HWND deviceStringHwnd = NULL;
	RECT topBorder;
	RECT bottomBorder;
	RECT borderBetweenDeviceNameAndString;
	RECT leftBorder;
	RECT rightBorder;
};

struct DeviceElement {
	DISPLAY_DEVICEW device;
	std::vector<MonitorElement> monitors;
	HWND deviceNameHwnd = NULL;
	HWND deviceStringHwnd = NULL;
	HWND rectStringHwnd = NULL;
	HWND workRectStringHwnd = NULL;
	HWND btnHwnd = NULL;
	RECT borderBetweenDeviceNameAndString;
	RECT borderBetweenDeviceStringAndRect;
	RECT borderBetweenRectAndWorkRect;
	RECT topBorder;
	RECT bottomBorder;
	RECT leftBorder;
	RECT rightBorder;
	DWORD iDevNum;
};
std::vector<DeviceElement> devices;
std::vector<MONITORINFOEXW> monitorInfos;

int longestDeviceName = 0;
int longestDeviceString = 0;
int textHeight = 0;
int longestMonitorName = 0;
int longestMonitorString = 0;
int longestRectString = 0;
int longestWorkRectString = 0;
int borderPadding = 3;
const wchar_t* DeviceNameTitleStr = L"Device Name";
const wchar_t* DeviceStringTitleStr = L"Device String";
const wchar_t* MonitorNameTitleStr = L"Monitor Name";
const wchar_t* MonitorStringTitleStr = L"Monitor String";
const wchar_t* RectTitleStr = L"Whole Rect";
const wchar_t* WorkRectTitleStr = L"Work Rect";
const wchar_t* MoveStr = L"Move Xrd Here";
int innerSpacing = 8;
int subrowIndent = 20;
int MoveStrSize = 0;
int MoveButtonPadX = 20;
int MoveButtonPadY = 8;

wchar_t* formatWorkRect(const RECT* rect) {
	swprintf_s(wstrbuf, L"X: %d; Y: %d; W: %d; H:%d;", rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
	return wstrbuf;
}

void onButtonPressed(const DeviceElement& dd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GGXRDCHANGEBORDERLESSWINDOWPOS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GGXRDCHANGEBORDERLESSWINDOWPOS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	
	GdiplusShutdown(gdiplusToken);
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GGXRDCHANGEBORDERLESSWINDOWPOS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GGXRDCHANGEBORDERLESSWINDOWPOS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   mainWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!mainWindow)
   {
      return FALSE;
   }
	
	hbrBkgnd = GetSysColorBrush(COLOR_WINDOW);
	NONCLIENTMETRICSW nonClientMetrics { 0 };
	nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICSW);
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &nonClientMetrics, NULL);
	font = CreateFontIndirectW(&nonClientMetrics.lfCaptionFont);
	
	RECT clientRect;
	GetClientRect(mainWindow, &clientRect);
	
	int padding = 5;
	HDC hdc = GetDC(mainWindow);
	HGDIOBJ oldObj = SelectObject(hdc, (HGDIOBJ)font);
	SIZE textSz{0};
	#define prepareSize(txt) { \
		const wchar_t* txtConst = txt; \
		GetTextExtentPoint32W(hdc, txtConst, (int)wcslen(txtConst), &textSz); \
	}
	
	DISPLAY_DEVICEW displayDevice;
	displayDevice.cb = sizeof DISPLAY_DEVICEW;
	DWORD iDevNum = 0;
	while (true) {
		// query all display devices in the current session
		BOOL enumResult = EnumDisplayDevicesW(NULL, iDevNum++, &displayDevice, 0);
		if (!enumResult) break;
		
		devices.emplace_back();
		DeviceElement& dd = devices.back();
		dd.iDevNum = iDevNum - 1;
		dd.device = displayDevice;
		
		// obtain information on a display monitor
		DISPLAY_DEVICEW monitorDevice;
		monitorDevice.cb = sizeof DISPLAY_DEVICEW;
		DWORD monitorNum = 0;
		while (true) {
			enumResult = EnumDisplayDevicesW(dd.device.DeviceName, monitorNum++, &monitorDevice, 0);
			if (!enumResult) break;
			dd.monitors.push_back({ monitorDevice });
		}
		
		if (monitorNum == 1) {
			devices.erase(devices.begin() + (devices.size() - 1));
			continue;
		}
		//HDC deviceHDC = CreateDCW(dd.device.DeviceName, dd.device.DeviceName, NULL, NULL);  to get HDC of the display device
		// does not work for a monitor, only the display device
		
		//GetDeviceCaps(deviceHDC, HORZRES);
		//GetDeviceCaps(deviceHDC, VERTRES); 
	}
	
	struct MyEnumProc {
		static BOOL CALLBACK Proc(HMONITOR hMonitor,
									HDC hdcMonitor,
									LPRECT lprcMonitor,
									LPARAM dwData) {
			monitorInfos.emplace_back();
			monitorInfos.back().cbSize = sizeof MONITORINFOEX;
			GetMonitorInfoW(hMonitor, &monitorInfos.back());
			return TRUE;
		}
	};
	EnumDisplayMonitors(NULL, NULL, MyEnumProc::Proc, NULL);
	
	for (const DeviceElement& dd : devices) {
		prepareSize(dd.device.DeviceName)
		if (textSz.cx > longestDeviceName) longestDeviceName = textSz.cx;
		prepareSize(dd.device.DeviceString)
		if (textSz.cx > longestDeviceString) longestDeviceString = textSz.cx;
		for (const MonitorElement& monitor : dd.monitors) {
			prepareSize(monitor.device.DeviceName)
			if (textSz.cx > longestMonitorName) longestMonitorName = textSz.cx;
			prepareSize(monitor.device.DeviceString)
			if (textSz.cx > longestMonitorString) longestMonitorString = textSz.cx;
		}
		for (const MONITORINFOEXW& monitorInfo : monitorInfos) {
			if (wcscmp(dd.device.DeviceName, monitorInfo.szDevice) == 0) {
				prepareSize(formatWorkRect(&monitorInfo.rcMonitor))
				if (textSz.cx > longestRectString) longestRectString = textSz.cx;
				prepareSize(formatWorkRect(&monitorInfo.rcWork))
				if (textSz.cx > longestWorkRectString) longestWorkRectString = textSz.cx;
			}
		}
	}
	
	prepareSize(DeviceNameTitleStr)
	if (textSz.cx > longestDeviceName) longestDeviceName = textSz.cx;
	textHeight = textSz.cy;
	prepareSize(DeviceStringTitleStr)
	if (textSz.cx > longestDeviceString) longestDeviceString = textSz.cx;
	prepareSize(MonitorNameTitleStr)
	if (textSz.cx > longestMonitorName) longestMonitorName = textSz.cx;
	prepareSize(MonitorStringTitleStr)
	if (textSz.cx > longestMonitorString) longestMonitorString = textSz.cx;
	prepareSize(RectTitleStr)
	if (textSz.cx > longestRectString) longestRectString = textSz.cx;
	prepareSize(WorkRectTitleStr)
	if (textSz.cx > longestWorkRectString) longestWorkRectString = textSz.cx;
	
	int x = padding;
	
	DeviceNameTitleHwnd = CreateWindowW(WC_STATICW, DeviceNameTitleStr,
		WS_CHILD
			| WS_OVERLAPPED
			| WS_VISIBLE,
		x,
		padding,
		longestDeviceName,
		textHeight,
		mainWindow, NULL, hInst, NULL);
	SendMessageW(DeviceNameTitleHwnd, WM_SETFONT, (WPARAM)font, TRUE);
	
	x += longestDeviceName + innerSpacing;
	
	DeviceStringTitleHwnd = CreateWindowW(WC_STATICW, DeviceStringTitleStr,
		WS_CHILD
			| WS_OVERLAPPED
			| WS_VISIBLE,
		x,
		padding,
		longestDeviceString,
		textHeight,
		mainWindow, NULL, hInst, NULL);
	SendMessageW(DeviceStringTitleHwnd, WM_SETFONT, (WPARAM)font, TRUE);
	
	x += longestDeviceString + innerSpacing;
	
	RectTitleHwnd = CreateWindowW(WC_STATICW, RectTitleStr,
		WS_CHILD
			| WS_OVERLAPPED
			| WS_VISIBLE,
		x,
		padding,
		longestRectString,
		textHeight,
		mainWindow, NULL, hInst, NULL);
	SendMessageW(RectTitleHwnd, WM_SETFONT, (WPARAM)font, TRUE);
	
	x += longestRectString + innerSpacing;
	
	WorkRectTitleHwnd = CreateWindowW(WC_STATICW, WorkRectTitleStr,
		WS_CHILD
			| WS_OVERLAPPED
			| WS_VISIBLE,
		x,
		padding,
		longestWorkRectString,
		textHeight,
		mainWindow, NULL, hInst, NULL);
	SendMessageW(WorkRectTitleHwnd, WM_SETFONT, (WPARAM)font, TRUE);
	
	x = padding + subrowIndent;
	int y = padding + textHeight + innerSpacing;
	
	MonitorNameTitleHwnd = CreateWindowW(WC_STATICW, MonitorNameTitleStr,
		WS_CHILD
			| WS_OVERLAPPED
			| WS_VISIBLE,
		x,
		y,
		longestMonitorName,
		textHeight,
		mainWindow, NULL, hInst, NULL);
	SendMessageW(MonitorNameTitleHwnd, WM_SETFONT, (WPARAM)font, TRUE);
	
	x += longestMonitorName + innerSpacing;
	
	MonitorStringTitleHwnd = CreateWindowW(WC_STATICW, MonitorStringTitleStr,
		WS_CHILD
			| WS_OVERLAPPED
			| WS_VISIBLE,
		x,
		y,
		longestMonitorString,
		textHeight,
		mainWindow, NULL, hInst, NULL);
	SendMessageW(MonitorStringTitleHwnd, WM_SETFONT, (WPARAM)font, TRUE);
	
	y += textHeight + innerSpacing;
	
	prepareSize(MoveStr)
	MoveStrSize = textSz.cx;
	
	for (DeviceElement& dd : devices) {
		int x = padding;
		
		dd.leftBorder.left = x - borderPadding;
		dd.leftBorder.right = dd.leftBorder.left;
		dd.leftBorder.top = y - 1;
		dd.leftBorder.bottom = y + textHeight + 1;
		dd.topBorder.left = dd.leftBorder.left;
		dd.topBorder.top = y - 1;
		dd.topBorder.bottom = y - 1;
		
		dd.bottomBorder.left = dd.leftBorder.left;
		dd.bottomBorder.top = y + textHeight + 1;
		dd.bottomBorder.bottom = dd.bottomBorder.top + 1;
		
		dd.deviceNameHwnd = CreateWindowW(WC_STATICW, dd.device.DeviceName,
			WS_CHILD
				| WS_OVERLAPPED
				| WS_VISIBLE,
			x,
			y,
			longestDeviceName,
			textHeight,
			mainWindow, NULL, hInst, NULL);
		whiteBgLabels.push_back(dd.deviceNameHwnd);
		SendMessageW(dd.deviceNameHwnd, WM_SETFONT, (WPARAM)font, TRUE);
		
		x += longestDeviceName + innerSpacing;
		
		dd.borderBetweenDeviceNameAndString.left = x - (innerSpacing >> 1);
		dd.borderBetweenDeviceNameAndString.right = dd.borderBetweenDeviceNameAndString.left;
		dd.borderBetweenDeviceNameAndString.top = y;
		dd.borderBetweenDeviceNameAndString.bottom = y + textHeight;
		
		dd.deviceStringHwnd = CreateWindowW(WC_STATICW, dd.device.DeviceString,
			WS_CHILD
				| WS_OVERLAPPED
				| WS_VISIBLE,
			x,
			y,
			longestDeviceString,
			textHeight,
			mainWindow, NULL, hInst, NULL);
		whiteBgLabels.push_back(dd.deviceStringHwnd);
		SendMessageW(dd.deviceStringHwnd, WM_SETFONT, (WPARAM)font, TRUE);
		
		x += longestDeviceString + innerSpacing;
		
		dd.borderBetweenDeviceStringAndRect.left = x - (innerSpacing >> 1);
		dd.borderBetweenDeviceStringAndRect.right = dd.borderBetweenDeviceStringAndRect.left;
		dd.borderBetweenDeviceStringAndRect.top = y;
		dd.borderBetweenDeviceStringAndRect.bottom = y + textHeight;
		
		for (const MONITORINFOEXW& monitorInfo : monitorInfos) {
			if (wcscmp(monitorInfo.szDevice, dd.device.DeviceName) == 0) {
				
				formatWorkRect(&monitorInfo.rcMonitor);
				
				dd.rectStringHwnd = CreateWindowW(WC_STATICW, wstrbuf,
					WS_CHILD
						| WS_OVERLAPPED
						| WS_VISIBLE,
					x,
					y,
					longestRectString,
					textHeight,
					mainWindow, NULL, hInst, NULL);
				whiteBgLabels.push_back(dd.rectStringHwnd);
				SendMessageW(dd.rectStringHwnd, WM_SETFONT, (WPARAM)font, TRUE);
				
				x += longestRectString + innerSpacing;
				
				dd.borderBetweenRectAndWorkRect.left = x - (innerSpacing >> 1);
				dd.borderBetweenRectAndWorkRect.right = dd.borderBetweenRectAndWorkRect.left;
				dd.borderBetweenRectAndWorkRect.top = y;
				dd.borderBetweenRectAndWorkRect.bottom = y + textHeight;
				
				formatWorkRect(&monitorInfo.rcWork);
				
				dd.workRectStringHwnd = CreateWindowW(WC_STATICW, wstrbuf,
					WS_CHILD
						| WS_OVERLAPPED
						| WS_VISIBLE,
					x,
					y,
					longestWorkRectString,
					textHeight,
					mainWindow, NULL, hInst, NULL);
				whiteBgLabels.push_back(dd.workRectStringHwnd);
				SendMessageW(dd.workRectStringHwnd, WM_SETFONT, (WPARAM)font, TRUE);
				
				x += longestWorkRectString + innerSpacing;
				
				dd.btnHwnd = CreateWindowW(WC_BUTTONW, MoveStr,
					WS_CHILD
						| WS_OVERLAPPED
						| WS_VISIBLE,
					x,
					y,
					MoveStrSize + MoveButtonPadX,
					textHeight + MoveButtonPadY,
					mainWindow, NULL, hInst, NULL);
				SendMessageW(dd.btnHwnd, WM_SETFONT, (WPARAM)font, TRUE);
				
			}
		}
		
		dd.rightBorder.top = y - 1;
		dd.rightBorder.bottom = y + textHeight + 1;
		dd.rightBorder.left = x - (innerSpacing >> 1);
		dd.rightBorder.right = dd.rightBorder.left;
		dd.topBorder.right = dd.rightBorder.left;
		dd.bottomBorder.right = dd.rightBorder.left;
		
		x = padding + subrowIndent;
		
		for (MonitorElement& monitor : dd.monitors) {
			
			y += textHeight + innerSpacing;
			
			monitor.leftBorder.left = x - borderPadding;
			monitor.leftBorder.right = monitor.leftBorder.left;
			monitor.leftBorder.top = y - 1;
			monitor.leftBorder.bottom = y + textHeight + 1;
			monitor.topBorder.left = monitor.leftBorder.left;
			monitor.topBorder.top = y - 1;
			monitor.topBorder.bottom = y - 1;
			monitor.bottomBorder.left = monitor.leftBorder.left;
			monitor.bottomBorder.top = y + textHeight + 1;
			monitor.bottomBorder.bottom = monitor.bottomBorder.top;
			
			monitor.deviceNameHwnd = CreateWindowW(WC_STATICW, monitor.device.DeviceName,
				WS_CHILD
					| WS_OVERLAPPED
					| WS_VISIBLE,
				x,
				y,
				longestMonitorName,
				textHeight,
				mainWindow, NULL, hInst, NULL);
			whiteBgLabels.push_back(monitor.deviceNameHwnd);
			SendMessageW(monitor.deviceNameHwnd, WM_SETFONT, (WPARAM)font, TRUE);
			
			x += longestMonitorName + innerSpacing;
			
			monitor.borderBetweenDeviceNameAndString.left = x - (innerSpacing >> 1);
			monitor.borderBetweenDeviceNameAndString.right = monitor.borderBetweenDeviceNameAndString.left;
			monitor.borderBetweenDeviceNameAndString.top = y;
			monitor.borderBetweenDeviceNameAndString.bottom = y + textHeight;
			
			monitor.deviceStringHwnd = CreateWindowW(WC_STATICW, monitor.device.DeviceString,
				WS_CHILD
					| WS_OVERLAPPED
					| WS_VISIBLE,
				x,
				y,
				longestMonitorString,
				textHeight,
				mainWindow, NULL, hInst, NULL);
			whiteBgLabels.push_back(monitor.deviceStringHwnd);
			SendMessageW(monitor.deviceStringHwnd, WM_SETFONT, (WPARAM)font, TRUE);
			
			x += longestMonitorString;
			
			monitor.rightBorder.left = x + borderPadding;
			monitor.rightBorder.right = monitor.rightBorder.left;
			monitor.rightBorder.top = y - 1;
			monitor.rightBorder.bottom = y + textHeight + 1;
			monitor.topBorder.right = monitor.rightBorder.left;
			monitor.bottomBorder.right = monitor.rightBorder.left;
			
		}
		
		y += textHeight + innerSpacing * 2;
		
	}
	
   ShowWindow(mainWindow, nCmdShow);
   UpdateWindow(mainWindow);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
        	for (const DeviceElement& dd : devices) {
        		if (dd.btnHwnd == (HWND)lParam) {
        			int notifCode = HIWORD(wParam);
        			if (notifCode == BN_CLICKED) {
        				onButtonPressed(dd);
        			}
        			break;
        		}
        	}
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            Graphics graphics(hdc);
			Pen pen(Color(255, 0, 0, 0));
            for (const DeviceElement& dd : devices) {
				graphics.DrawLine(&pen, (INT)dd.borderBetweenDeviceNameAndString.left,
					(INT)dd.borderBetweenDeviceNameAndString.top,
					(INT)dd.borderBetweenDeviceNameAndString.right,
					(INT)dd.borderBetweenDeviceNameAndString.bottom);
				graphics.DrawLine(&pen, (INT)dd.borderBetweenDeviceStringAndRect.left,
					(INT)dd.borderBetweenDeviceStringAndRect.top,
					(INT)dd.borderBetweenDeviceStringAndRect.right,
					(INT)dd.borderBetweenDeviceStringAndRect.bottom);
				graphics.DrawLine(&pen, (INT)dd.borderBetweenRectAndWorkRect.left,
					(INT)dd.borderBetweenRectAndWorkRect.top,
					(INT)dd.borderBetweenRectAndWorkRect.right,
					(INT)dd.borderBetweenRectAndWorkRect.bottom);
				graphics.DrawLine(&pen, (INT)dd.topBorder.left,
					(INT)dd.topBorder.top,
					(INT)dd.topBorder.right,
					(INT)dd.topBorder.bottom);
				graphics.DrawLine(&pen, (INT)dd.bottomBorder.left,
					(INT)dd.bottomBorder.top,
					(INT)dd.bottomBorder.right,
					(INT)dd.bottomBorder.bottom);
				graphics.DrawLine(&pen, (INT)dd.leftBorder.left,
					(INT)dd.leftBorder.top,
					(INT)dd.leftBorder.right,
					(INT)dd.leftBorder.bottom);
				graphics.DrawLine(&pen, (INT)dd.rightBorder.left,
					(INT)dd.rightBorder.top,
					(INT)dd.rightBorder.right,
					(INT)dd.rightBorder.bottom);
				for (const MonitorElement& monitor : dd.monitors) {
					graphics.DrawLine(&pen, (INT)monitor.borderBetweenDeviceNameAndString.left,
						(INT)monitor.borderBetweenDeviceNameAndString.top,
						(INT)monitor.borderBetweenDeviceNameAndString.right,
						(INT)monitor.borderBetweenDeviceNameAndString.bottom);
					graphics.DrawLine(&pen, (INT)monitor.topBorder.left,
						(INT)monitor.topBorder.top,
						(INT)monitor.topBorder.right,
						(INT)monitor.topBorder.bottom);
					graphics.DrawLine(&pen, (INT)monitor.bottomBorder.left,
						(INT)monitor.bottomBorder.top,
						(INT)monitor.bottomBorder.right,
						(INT)monitor.bottomBorder.bottom);
					graphics.DrawLine(&pen, (INT)monitor.leftBorder.left,
						(INT)monitor.leftBorder.top,
						(INT)monitor.leftBorder.right,
						(INT)monitor.leftBorder.bottom);
					graphics.DrawLine(&pen, (INT)monitor.rightBorder.left,
						(INT)monitor.rightBorder.top,
						(INT)monitor.rightBorder.right,
						(INT)monitor.rightBorder.bottom);
				}
            }
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_CTLCOLORSTATIC: {
		if (std::find(whiteBgLabels.begin(), whiteBgLabels.end(), (HWND)lParam) != whiteBgLabels.end()) {
			return (INT_PTR)hbrBkgnd;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

inline DWORD ptrToRva(BYTE* ptr);

int calculateRelativeOffset(BYTE* callInstr, BYTE* callDest) {
	ULONG_PTR callInstrOff = callInstr - fileBase;
	ULONG_PTR callDestOff = callDest - fileBase;
	return (int)ptrToRva(callDest) - (int)(ptrToRva(callInstr) + 5);
}

inline BYTE* rvaToPtr(DWORD rva);

BYTE* followRelativeCall(BYTE* callInstr) {
	return rvaToPtr(
		ptrToRva(callInstr) + 5 + *(int*)(callInstr + 1)
	);
}

int findLast(const std::vector<wchar_t>& str, wchar_t character) {
    if (str.empty() || str.size() == 1) return -1;
    auto it = str.cend();
    --it;
    --it;
    while (true) {
        if (*it == character) return (int)(it - str.cbegin());
        if (it == str.cbegin()) return -1;
        --it;
    }
    return -1;
}

// Does not include trailing slash
std::vector<wchar_t> getParentDir(const std::vector<wchar_t>& path) {
    std::vector<wchar_t> result;
    int lastSlashPos = findLast(path, L'\\');
    if (lastSlashPos == -1) return result;
    result.assign(path.cbegin(), path.cbegin() + lastSlashPos + 1);
    result[result.size() - 1] = L'\0';
    return result;
}

std::vector<wchar_t> getFileName(const std::vector<wchar_t>& path) {
    std::vector<wchar_t> result;
    int lastSlashPos = findLast(path, L'\\');
    if (lastSlashPos == -1) return path;
    result.assign(path.cbegin() + lastSlashPos + 1, path.cend());
    return result;
}

bool fileExists(const wchar_t* path) {
	return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

inline DWORD vaToRva(DWORD va, DWORD imageBase) { return va - imageBase; }
inline DWORD rvaToVa(DWORD rva, DWORD imageBase) { return rva + imageBase; }

DWORD rvaToRaw(DWORD rva) {
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNtHeader);
	for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; ++i) {
		if (rva >= section->VirtualAddress && (
				i == pNtHeader->FileHeader.NumberOfSections - 1
					? true
					: rva < (section + 1)->VirtualAddress
			)) {
			return rva - section->VirtualAddress + section->PointerToRawData;
		}
		++section;
	}
	return 0;
}

DWORD rawToRva(DWORD raw) {
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNtHeader);
	PIMAGE_SECTION_HEADER maxSection = section;
	for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; ++i) {
		if (raw >= section->PointerToRawData && raw < section->PointerToRawData + section->SizeOfRawData) {
			return raw - section->PointerToRawData + section->VirtualAddress;
		}
		if (section->VirtualAddress > maxSection->VirtualAddress) maxSection = section;
		section++;
	}
	return raw - maxSection->PointerToRawData + maxSection->VirtualAddress;
}

DWORD vaToRaw(DWORD va) {
	return rvaToRaw(vaToRva(va, (DWORD)pNtHeader->OptionalHeader.ImageBase));
}

DWORD rawToVa(DWORD raw) {
	return rvaToVa(rawToRva(raw), (DWORD)pNtHeader->OptionalHeader.ImageBase);
}

inline DWORD ptrToRaw(BYTE* ptr) { return (DWORD)(ptr - fileBase); }
inline DWORD ptrToRva(BYTE* ptr) { return rawToRva(ptrToRaw(ptr)); }
inline DWORD ptrToVa(BYTE* ptr) { return rawToVa(ptrToRaw(ptr)); }
inline BYTE* rawToPtr(DWORD raw) { return raw + fileBase; }
inline BYTE* rvaToPtr(DWORD rva) { return rawToPtr(rvaToRaw(rva)); }
inline BYTE* vaToPtr(DWORD va) { return rawToPtr(vaToRaw(va)); }

struct FoundReloc {
	char type;  // see macros starting with IMAGE_REL_BASED_
	DWORD regionVa;  // position of the place that the reloc is patching
	DWORD relocVa;  // position of the reloc entry itself
	BYTE* ptr;  // points to the page base member of the block
	bool ptrIsLast;  // block is the last in the table
};

struct FoundRelocBlock {
	BYTE* ptr;  // points to the page base member of the block
	DWORD pageBaseVa;  // page base of all patches that the reloc is responsible for
	DWORD relocVa;  // position of the reloc block itself. Points to the page base member of the block
	DWORD size;  // size of the entire block, including the page base and block size and all entries
	bool isLast;  // this block is the last in the table
};

struct RelocBlockIterator {
	
	BYTE* const relocTableOrig;
	const DWORD relocTableVa;
	const DWORD imageBase;
	DWORD relocTableSize;  // remaining size
	BYTE* relocTableNext;
	
	RelocBlockIterator(BYTE* relocTable, DWORD relocTableVa, DWORD relocTableSize, DWORD imageBase)
		:
		relocTableOrig(relocTable),
		relocTableVa(relocTableVa),
		imageBase(imageBase),
		relocTableSize(relocTableSize),
		relocTableNext(relocTable) { }
		
	bool getNext(FoundRelocBlock& block) {
		
		if (relocTableSize == 0) return false;
		
		BYTE* relocTable = relocTableNext;
		
		DWORD pageBaseRva = *(DWORD*)relocTable;
		DWORD pageBaseVa = rvaToVa(pageBaseRva, imageBase);
		DWORD blockSize = *(DWORD*)(relocTable + 4);
		
		relocTableNext += blockSize;
		
		if (relocTableSize <= blockSize) relocTableSize = 0;
		else relocTableSize -= blockSize;
		
		block.ptr = relocTable;
		block.pageBaseVa = pageBaseVa;
		block.relocVa = relocTableVa + (DWORD)(uintptr_t)(relocTable - relocTableOrig);
		block.size = blockSize;
		block.isLast = relocTableSize == 0;
		return true;
	}
};

struct RelocEntryIterator {
	const DWORD blockVa;
	BYTE* const blockStart;
	const BYTE* ptr;  // the pointer to the next entry
	DWORD blockSize;  // the remaining, unconsumed size of the block (we don't actually modify any data, so maybe consume is not the right word)
	const DWORD pageBaseVa;
	const bool blockIsLast;
	
	/// <param name="ptr">The address of the start of the whole reloc block, NOT the first member.</param>
	/// <param name="blockSize">The size of the entire reloc block, in bytes, including the page base member and the block size member.</param>
	/// <param name="pageBaseVa">The page base of the reloc block.</param>
	/// <param name="blockVa">The Virtual Address where the reloc block itself is located. Must point to the page base member of the block.</param>
	RelocEntryIterator(BYTE* ptr, DWORD blockSize, DWORD pageBaseVa, DWORD blockVa, bool blockIsLast)
		:
		ptr(ptr + 8),
		blockSize(blockSize <= 8 ? 0 : blockSize - 8),
		pageBaseVa(pageBaseVa),
		blockStart(ptr),
		blockVa(blockVa),
		blockIsLast(blockIsLast) { }
		
	RelocEntryIterator(const FoundRelocBlock& block) : RelocEntryIterator(
		block.ptr, block.size, block.pageBaseVa, block.relocVa, block.isLast) { }
	
	bool getNext(FoundReloc& reloc) {
		if (blockSize == 0) return false;
		
		unsigned short entry = *(unsigned short*)ptr;
		char blockType = (entry & 0xF000) >> 12;
		
		DWORD entrySize = blockType == IMAGE_REL_BASED_HIGHADJ ? 4 : 2;
		
		reloc.type = blockType;
		reloc.regionVa = pageBaseVa | (entry & 0xFFF);
		reloc.relocVa = blockVa + (DWORD)(uintptr_t)(ptr - blockStart);
		reloc.ptr = blockStart;
		reloc.ptrIsLast = blockIsLast;
		
		if (blockSize <= entrySize) blockSize = 0;
		else blockSize -= entrySize;
		
		ptr += entrySize;
		
		return true;
	}
	
};

struct RelocTable {
	
	BYTE* relocTable;  // the pointer pointing to the reloc table's start. Typically that'd be the page base member of the first block
	DWORD va;  // Virtual Address of the reloc table's start
	DWORD raw;  // the raw position of the reloc table's start
	DWORD size;  // the size of the whole reloc table
	int sizeWhereRaw;  // the raw location of the size of the whole reloc table
	DWORD imageBase;  // the Virtual Address of the base of the image
	
	// Finds the file position of the start of the reloc table and its size
	void findRelocTable() {
		
	    IMAGE_DATA_DIRECTORY& relocDir = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	    DWORD relocRva = relocDir.VirtualAddress;
	    DWORD* relocSizePtr = &relocDir.Size;
	    sizeWhereRaw = (int)(uintptr_t)((BYTE*)relocSizePtr - fileBase);
	    
	    va = rvaToVa(relocRva, (DWORD)pNtHeader->OptionalHeader.ImageBase);
	    raw = rvaToRaw(relocRva);
	    relocTable = fileBase + raw;
	    size = *relocSizePtr;
	    imageBase = (DWORD)pNtHeader->OptionalHeader.ImageBase;
	}
	
	// region specified in Virtual Address space
	std::vector<FoundReloc> findRelocsInRegion(DWORD regionStart, DWORD regionEnd) const {
		std::vector<FoundReloc> result;
		
		RelocBlockIterator blockIterator(relocTable, va, size, imageBase);
		
		FoundRelocBlock block;
		while (blockIterator.getNext(block)) {
			if (block.pageBaseVa >= regionEnd || (block.pageBaseVa | 0xFFF) + 8 < regionStart) {
				continue;
			}
			RelocEntryIterator entryIterator(block);
			FoundReloc reloc;
			while (entryIterator.getNext(reloc)) {
				if (reloc.type == IMAGE_REL_BASED_ABSOLUTE) continue;
				DWORD patchVa = reloc.regionVa;
				DWORD patchSize = 4;
				
				switch (reloc.type) {
					case IMAGE_REL_BASED_HIGH: patchVa += 2; patchSize = 2; break;
					case IMAGE_REL_BASED_LOW: patchSize = 2; break;
					case IMAGE_REL_BASED_HIGHLOW: break;
					case IMAGE_REL_BASED_HIGHADJ:
						patchSize = 2;
						break;
					case IMAGE_REL_BASED_DIR64: patchSize = 8; break;
				}
				
				if (patchVa >= regionEnd || patchVa + patchSize < regionStart) continue;
				
				result.push_back(reloc);
			}
		}
		return result;
	}
	inline std::vector<FoundReloc> findRelocsInRegion(BYTE* regionStart, BYTE* regionEnd) const {
		return findRelocsInRegion(ptrToVa(regionStart), ptrToVa(regionEnd));
	}
	
	FoundRelocBlock findLastRelocBlock() const {
		
		RelocBlockIterator blockIterator(relocTable, va, size, imageBase);
		
		FoundRelocBlock block;
		while (blockIterator.getNext(block));
		return block;
	}
	
	// returns empty, unused entries that could potentially be reused for the desired vaToPatch
	std::vector<FoundReloc> findReusableRelocEntries(DWORD vaToPatch) const {
		std::vector<FoundReloc> result;
		
		RelocBlockIterator blockIterator(relocTable, va, size, imageBase);
		
		FoundRelocBlock block;
		while (blockIterator.getNext(block)) {
			
			if (!(
				block.pageBaseVa <= vaToPatch && vaToPatch <= (block.pageBaseVa | 0xFFF)
			)) {
				continue;
			}
			
			RelocEntryIterator entryIterator(block);
			FoundReloc reloc;
			while (entryIterator.getNext(reloc)) {
				if (reloc.type == IMAGE_REL_BASED_ABSOLUTE) {
					result.push_back(reloc);
				}
			}
		}
		
		return result;
	}
	
	template<typename T>
	void write(HANDLE file, int filePosition, T value) {
		SetFilePointer(file, filePosition, NULL, FILE_BEGIN);
		DWORD bytesWritten = 0;
		WriteFile(file, &value, sizeof(T), &bytesWritten, NULL);
	}
	
	template<typename T, size_t size>
	void write(HANDLE file, int filePosition, T (&value)[size]) {
		SetFilePointer(file, filePosition, NULL, FILE_BEGIN);
		DWORD bytesWritten = 0;
		WriteFile(file, value, sizeof(T) * size, &bytesWritten, NULL);
	}
	
	// Resize the whole reloc table.
	// Writes both into the file and into the size member.
	void increaseSizeBy(HANDLE file, DWORD n) {
		
	    DWORD relocSizeRoundUp = (size + 3) & ~3;
		size = relocSizeRoundUp + n;  // reloc size includes the page base VA and the size of the reloc table entry and of all the entries
		// but I still have to round it up to 32 bits
		
		write(file, sizeWhereRaw, size);
		
	}
	void decreaseSizeBy(HANDLE file, DWORD n) {
		
		size -= n;  // reloc size includes the page base VA and the size of the reloc table entry and of all the entries
		// but I still have to round it up to 32 bits
		
		write(file, sizeWhereRaw, size);
		
	}
	
	// Try to:
	// 1) Reuse an existing 0000 entry that has a page base from which we can reach the target;
	// 2) Try to expand the last block if the target is reachable from its page base;
	// 3) Add a new block to the end of the table with that one entry.
	void addEntry(HANDLE file, DWORD vaToRelocate, char type) {
		unsigned short relocEntry = ((unsigned short)type << 12) | (vaToRva(vaToRelocate, imageBase) & 0xFFF);
		
		std::vector<FoundReloc> reusableEntries = findReusableRelocEntries(vaToRelocate);
		if (!reusableEntries.empty()) {
			const FoundReloc& firstReloc = reusableEntries.front();
			write(file, firstReloc.relocVa - va + raw, relocEntry);
			*(unsigned short*)(relocTable + firstReloc.relocVa - va) = relocEntry;
			return;
		}
		
		const FoundRelocBlock lastBlock = findLastRelocBlock();
		if (lastBlock.pageBaseVa <= vaToRelocate && vaToRelocate <= (lastBlock.pageBaseVa | 0xFFF)) {
			DWORD newSize = lastBlock.size + 2;
			newSize  = (newSize + 3) & ~3;  // round the size up to 32 bits
			
			DWORD oldTableSize = size;
			DWORD sizeIncrease = newSize - lastBlock.size;
			increaseSizeBy(file, sizeIncrease);
			
			int pos = lastBlock.relocVa - va + raw;
			write(file, pos + 4, newSize);
			*(DWORD*)(relocTable + lastBlock.relocVa - va + 4) = newSize;
			
			write(file, pos + lastBlock.size, relocEntry);
			unsigned short* newEntryPtr = (unsigned short*)(relocTable + lastBlock.relocVa - va + lastBlock.size);
			*newEntryPtr = relocEntry;
			
			if (sizeIncrease > 2) {
				unsigned short zeros = 0;
				write(file, pos + lastBlock.size + 2, zeros);
				*(newEntryPtr + 1) = zeros;
			}
			
			return;
		}
		
		DWORD oldSize = size;
	    // "Each block must start on a 32-bit boundary." - Microsoft
		DWORD oldSizeRoundedUp = (oldSize + 3) & ~3;
		
		// add a new reloc table with one entry
		increaseSizeBy(file, 12);  // changes 'size'
		
		
		DWORD rvaToRelocate = vaToRva(vaToRelocate, imageBase);
		DWORD newRelocPageBase = rvaToRelocate & 0xFFFFF000;
		
		DWORD tableData[3];
		tableData[0] = newRelocPageBase;
		tableData[1] = 12;  // page base (4) + size of whole block (4) + one entry (2) + padding to make it 32-bit complete (2)
		tableData[2] = relocEntry;
		
		write(file, raw + oldSizeRoundedUp, tableData);
		memcpy(relocTable + oldSizeRoundedUp, tableData, sizeof tableData);
		
	}
	inline void addEntry(HANDLE file, BYTE* ptrToRelocate, char type) {
		addEntry(file, ptrToVa(ptrToRelocate), type);
	}
	
	// fills entry with zeros, diabling it. Does not change the size of either the reloc table or the reloc block
	void removeEntry(HANDLE file, const FoundReloc& reloc) {
		unsigned short zeros = 0;
		int relocRaw = reloc.relocVa - va + raw;
		int relocOffset = reloc.relocVa - va;
		write(file, relocRaw, zeros);
		*(unsigned short*)(relocTable + reloc.relocVa - va) = zeros;
		if (reloc.ptrIsLast) {
			DWORD& size = *(
				(DWORD*)reloc.ptr + 1
			);
			int count = 0;
			int lastOffset = size - 2;
			while (lastOffset >= 8) {
				if (*(unsigned short*)(reloc.ptr + lastOffset) != 0) {
					break;
				}
				lastOffset -= 2;
				++count;
			}
			if (lastOffset < 8) {
				// delete whole block
				BYTE zeros[8] { 0 };
				int target = ptrToRaw(reloc.ptr);
				int sizeLeft = size;
				SetFilePointer(file, target, NULL, FILE_BEGIN);
				while (sizeLeft > 0) {
					int toWrite = sizeLeft > 8 ? 8 : sizeLeft;
					DWORD bytesWritten;
					WriteFile(file, zeros, toWrite, &bytesWritten, NULL);
					sizeLeft -= toWrite;
				}
				decreaseSizeBy(file, size);
			} else {
				int shrinkage = 0;
				while (count >= 2) {
					shrinkage += 2;
					count -= 2;
				}
				// shrink block by 4 bytes
				size -= shrinkage;
				decreaseSizeBy(file, shrinkage << 1);
			}
		}
	}
	
};

void onButtonPressed(const DeviceElement& dd) {
	HWND ggHwnd = NULL;
	DWORD gg = findOpenGgProcess(&ggHwnd);
	#define showError(fmtString, ...) { \
		swprintf_s(wstrbuf, fmtString, __VA_ARGS__); \
		MessageBoxW(mainWindow, wstrbuf, L"Error", MB_OK); \
	}
	if (gg) {
		
		HANDLE ggProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, gg);
		if (ggProc) {
			HMODULE ggModule;
			DWORD bytesNeeded = 0;
			if (EnumProcessModules(ggProc, &ggModule, sizeof HMODULE, &bytesNeeded)) {
				GetModuleFileNameExW(ggProc, ggModule, snatchedXrdPath, sizeof snatchedXrdPath);
			}
			CloseHandle(ggProc);
		}
		snatchedXrdPath[_countof(snatchedXrdPath) - 1] = L'\0';
		
		showError(L"Guilty Gear Xrd is open. It is only possible to patch it when it is not. Please close it and try again.")
		return;
	}
	for (const MONITORINFOEXW& monitorInfo : monitorInfos) {
		if (wcscmp(monitorInfo.szDevice, dd.device.DeviceName) == 0) {
			
			std::vector<WCHAR> szFile(MAX_PATH);
			
			if (snatchedXrdPath[0] != L'\0') {
				DWORD fileAttrib = GetFileAttributesW(snatchedXrdPath);
				if (fileAttrib == INVALID_FILE_ATTRIBUTES
						|| (fileAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
					snatchedXrdPath[0] = L'\0';
				} else {
					int len = (int)wcslen(snatchedXrdPath);
					static const wchar_t xrdname[] { L"\\GuiltyGearXrd.exe" };
					const int xrdnameLen = _countof(xrdname) - 1;
					if (!(
						len >= xrdnameLen && wcscmp(snatchedXrdPath + len - xrdnameLen, xrdname) == 0
					)) {
						snatchedXrdPath[0] = L'\0';
					}
				}
			}
			
			if (snatchedXrdPath[0] == L'\0') {
				
			    OPENFILENAMEW selectedFiles{ 0 };
			    selectedFiles.lStructSize = sizeof(OPENFILENAMEW);
			    selectedFiles.hwndOwner = NULL;
			    selectedFiles.lpstrFile = szFile.data();
			    selectedFiles.lpstrFile[0] = L'\0';
			    selectedFiles.nMaxFile = (DWORD)szFile.size() + 1;
				wchar_t filter[] = L"Windows Executable\0*.EXE\0";
			    selectedFiles.lpstrFilter = filter;
			    selectedFiles.nFilterIndex = 1;
			    selectedFiles.lpstrFileTitle = NULL;
			    selectedFiles.nMaxFileTitle = 0;
			    selectedFiles.lpstrInitialDir = NULL;
			    selectedFiles.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			
			    if (!GetOpenFileNameW(&selectedFiles)) {
			        DWORD errCode = CommDlgExtendedError();
			        if (errCode) {
			        	showError(L"Error selecting file. Error code: 0x%x", errCode)
			        }
			        return;
			    }
			    szFile.resize(lstrlenW(szFile.data()) + 1);
			} else {
				memcpy(szFile.data(), snatchedXrdPath, MAX_PATH * sizeof WCHAR);
			}
			
			std::vector<wchar_t> fileName = getFileName(szFile);
			std::vector<wchar_t> fileNameNoDot;
		    std::vector<wchar_t> parentDir = getParentDir(szFile);
			std::vector<wchar_t> backupPath;
			
			int dotPos = findLast(fileName, L'.');
			if (dotPos == -1) {
				showError(L"Chosen file name does not end with '.EXE': %ls", fileName.data())
				return;
			}
			
			fileNameNoDot.assign(fileName.begin(), fileName.begin() + dotPos + 1);
			fileNameNoDot[fileNameNoDot.size() - 1] = L'\0';
			
			int len = swprintf_s(wstrbuf, L"%ls\\%ls_backup%ls", parentDir.data(), fileNameNoDot.data(), fileName.data() + dotPos);
			
		    int backupNameCounter = 1;
		    while (fileExists(wstrbuf)) {
		    	len = swprintf_s(wstrbuf, L"%ls\\%ls_backup%d%ls", parentDir.data(), fileNameNoDot.data(), backupNameCounter, fileName.data() + dotPos);
		        ++backupNameCounter;
		    }
		    
			backupPath.resize(len + 1);
			memcpy(backupPath.data(), wstrbuf, (len + 1) * sizeof (wchar_t));
			
			if (!CopyFileW(szFile.data(), backupPath.data(), TRUE)) {
				WinError winErr;
				showError(L"Failed to create a backup copy of GuiltyGearXrd.exe at '%ls': %ls", backupPath.data(), winErr.getMessage())
				return;
			}
			
			swprintf_s(wstrbuf, L"Created a backup copy of GuiltyGearXrd.exe at '%ls'."
					L"You will be able to go to a state before patching by replacing Xrd with this file.", backupPath.data());
			int response = MessageBoxW(mainWindow, wstrbuf, L"Info", MB_OKCANCEL);
			if (response == IDCANCEL) return;
			
			HANDLE file = CreateFileW(szFile.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file == INVALID_HANDLE_VALUE) {
				WinError winErr;
				showError(L"Could not open EXE file: %ls", winErr.getMessage())
				return;
			}
			
			struct FileCloser {
				~FileCloser() {
					if (file) {
						CloseHandle(file);
					}
					if (!success) {
						if (DeleteFileW(szFile) && CopyFileW(backupPath, szFile, TRUE)) {
							MessageBoxW(mainWindow, L"Old EXE file has been restored from backup and the backup, as it is no longer needed, is removed."
								L" The entirety of patching has been undone.", L"Info", MB_OK);
						}
					}
				}
				HANDLE file = NULL;
				const wchar_t* szFile;
				const wchar_t* backupPath;
				bool success = false;
			} fileCloser{file, szFile.data(), backupPath.data()};
			
			DWORD fileSize = GetFileSize(file, NULL);
			DWORD newSectionSize = 0x1000;
			std::vector<BYTE> fileData(fileSize + newSectionSize);
			DWORD bytesRemaining = fileSize;
			BYTE* ptr = fileData.data();
			while (bytesRemaining) {
				DWORD bytesToRead = bytesRemaining <= 1024 ? bytesRemaining : 1024;
				DWORD bytesRead = 0;
				if (!ReadFile(file, ptr, bytesToRead, &bytesRead, NULL)) {
					WinError winErr;
					showError(L"Could not read EXE file: %ls", winErr.getMessage())
					return;
				}
				ptr += bytesToRead;
				bytesRemaining -= bytesToRead;
				if (bytesRead < bytesToRead) break;
			}
			
			fileBase = fileData.data();
			PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)fileBase;
			pNtHeader = (nthdr)((PBYTE)fileBase + pDosHeader->e_lfanew);
			DWORD bytesWritten = 0;
			
			#define fileOffset(addr) (LONG)((BYTE*)addr - fileBase)
			#define seekTo(addr) SetFilePointer(file, fileOffset(addr), NULL, FILE_BEGIN);
			#define write(buf, count) if (!WriteFile(file, buf, count, &bytesWritten, NULL)) { \
				WinError winErr; \
				showError(L"Could not write to EXE file: %ls", winErr.getMessage()) \
				return; \
			}
			#define writeAt(addr, buf, count) { \
				seekTo(addr) \
				write(buf, count) \
			}
			
			#define sectionStartEnd(varname, sectionName) \
				BYTE* varname##Ptr = nullptr; \
				BYTE* varname##PtrEnd = nullptr; \
				uintptr_t varname##VA = 0; \
				uintptr_t varname##VAEnd = 0; \
				PIMAGE_SECTION_HEADER varname##Section = nullptr; \
				{ \
					PIMAGE_SECTION_HEADER sectionSeek = IMAGE_FIRST_SECTION(pNtHeader); \
					for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; ++i) { \
						if (strncmp((const char*)sectionSeek->Name, sectionName, 8) == 0) { \
							varname##Ptr = fileBase + sectionSeek->PointerToRawData; \
							varname##PtrEnd = varname##Ptr + sectionSeek->SizeOfRawData; \
							varname##VA = (ULONG_PTR)pNtHeader->OptionalHeader.ImageBase + sectionSeek->VirtualAddress; \
							varname##VAEnd = varname##VA + sectionSeek->Misc.VirtualSize; \
							varname##Section = sectionSeek; \
							break; \
						} \
						++sectionSeek; \
					} \
				}
			
			sectionStartEnd(text, ".text")
			sectionStartEnd(existingSection, OUR_SECTION_NAME)
			
			PIMAGE_SECTION_HEADER newSectionPtr;
			BYTE* newSectionStart;
			if (!existingSectionPtr) {
				
				unsigned short numSections = pNtHeader->FileHeader.NumberOfSections + 1;
				writeAt(&pNtHeader->FileHeader.NumberOfSections, &numSections, 2)
				
				DWORD sizeOfImage = pNtHeader->OptionalHeader.SizeOfImage + newSectionSize;
				writeAt(&pNtHeader->OptionalHeader.SizeOfImage, &sizeOfImage, 4)
				
				newSectionPtr = IMAGE_FIRST_SECTION(pNtHeader)
					+ pNtHeader->FileHeader.NumberOfSections;
				
				DWORD prevSize = (newSectionPtr - 1)->SizeOfRawData;
				// round it up to nearest 0x100
				prevSize = (prevSize + 0xff) & (~0xff);
				
				newSectionStart = fileBase + (newSectionPtr - 1)->PointerToRawData + prevSize;
				
				memcpy(newSectionPtr->Name, OUR_SECTION_NAME, 8);
				newSectionPtr->Misc.PhysicalAddress = newSectionSize;  // supposed to be size without padding to next section. We'll just include padding
				newSectionPtr->VirtualAddress = pNtHeader->OptionalHeader.SizeOfImage;
				newSectionPtr->SizeOfRawData = newSectionSize;
				newSectionPtr->PointerToRawData = (DWORD)(newSectionStart - fileBase);
				newSectionPtr->PointerToLinenumbers = 0;
				newSectionPtr->NumberOfRelocations = 0;
				newSectionPtr->NumberOfLinenumbers = 0;
				newSectionPtr->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
				writeAt(newSectionPtr, newSectionPtr, sizeof IMAGE_SECTION_HEADER)
				
				++pNtHeader->FileHeader.NumberOfSections;
				
			} else {
				newSectionPtr = existingSectionSection;
				newSectionStart = existingSectionPtr;
			}
			
			BYTE* FWindowsViewportConstructor = nullptr;
			BYTE* pos = sigscanOffset(textPtr, textPtrEnd,
				"89 9e a8 04 00 00 89 9e e4 04 00 00 89 be e8 04 00 00 89 46 70",
				nullptr, "FWindowsViewportConstructor");
			if (pos) {
				FWindowsViewportConstructor = sigscanBackwards16ByteAligned(pos,
					"6a ff 68 ?? ?? ?? ?? 64 a1 00 00 00 00", 0xea);
				if (!FWindowsViewportConstructor) {
					FWindowsViewportConstructor = sigscanBackwards16ByteAligned(pos,
					"e9 ?? ?? ?? ?? 90 90 64 a1 00 00 00 00", 0xea);
					if (followRelativeCall(FWindowsViewportConstructor) != existingSectionPtr || !existingSectionPtr) {
						showError(L"FWindowsViewport::Constructor has already been modified by an unknown patcher.")
						return;
					}
				}
			}
			if (!FWindowsViewportConstructor) {
				showError(L"Could not find FWindowsViewport::Constructor")
				return;
			}
			
			#define movespinstr(varname, esp_offset, value) \
				const DWORD g982r2_##varname = (DWORD)(value); \
				BYTE varname[] { 0xC7, 0x44, 0x24, (BYTE)(esp_offset), \
					(BYTE)(g982r2_##varname & 0xFF), \
					(BYTE)((g982r2_##varname >> 8) & 0xFF), \
					(BYTE)((g982r2_##varname >> 16) & 0xFF), \
					(BYTE)((g982r2_##varname >> 24) & 0xFF) };
			
			//movespinstr(sizeXOverride, 0x10, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left)
			//movespinstr(sizeYOverride, 0x14, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top)
			movespinstr(posXOverride, 0x20, monitorInfo.rcMonitor.left)
			movespinstr(posYOverride, 0x24, monitorInfo.rcMonitor.top)
			
			BYTE oldConstructorCode[7];
			
			bool freshCode = FWindowsViewportConstructor[0] != 0xE9;
			
			RelocTable relocTable;
			relocTable.findRelocTable();
			
			if (freshCode) {
				
				std::vector<FoundReloc> relocs = relocTable.findRelocsInRegion(FWindowsViewportConstructor, FWindowsViewportConstructor + 7);
				
				for (const FoundReloc& foundReloc : relocs) {
					relocTable.removeEntry(file, foundReloc);
				}
				
				memcpy(oldConstructorCode, FWindowsViewportConstructor, sizeof oldConstructorCode);
			} else {
				memcpy(oldConstructorCode,
					existingSectionPtr
						+ sizeof posXOverride
						+ sizeof posYOverride,
					sizeof oldConstructorCode);
				
				std::vector<FoundReloc> relocs = relocTable.findRelocsInRegion(existingSectionPtr, existingSectionPtr + newSectionSize);
				
				for (const FoundReloc& foundReloc : relocs) {
					relocTable.removeEntry(file, foundReloc);
				}
				
			}
			
			BYTE newConstructorCode[7] {
				0xE9, 0x00, 0x00, 0x00, 0x00,  // JMP imm32
				0x90, 0x90
			};
			int offset = calculateRelativeOffset(FWindowsViewportConstructor, newSectionStart);
			memcpy(newConstructorCode + 1, &offset, 4);
			writeAt(FWindowsViewportConstructor, newConstructorCode, 7)
			
			seekTo(newSectionStart)
			//write(sizeXOverride, sizeof sizeXOverride)
			//write(sizeYOverride, sizeof sizeYOverride)
			write(posXOverride, sizeof posXOverride)
			write(posYOverride, sizeof posYOverride)
			
			#define getFilePos() (fileBase + SetFilePointer(file, 0, NULL, FILE_CURRENT))
			
			BYTE* relocateThis = getFilePos() + 3;
			write(oldConstructorCode, 7)
			
			BYTE* filePos = getFilePos();
			
			BYTE jumpBackInstr[5] {
				0xE9, 0x00, 0x00, 0x00, 0x00
			};
			offset = calculateRelativeOffset(filePos, FWindowsViewportConstructor + 7);
			memcpy(jumpBackInstr + 1, &offset, 4);
			write(jumpBackInstr, 5)
			
			BYTE* moreSectionCode = getFilePos();
			
			relocTable.addEntry(file, relocateThis, IMAGE_REL_BASED_HIGHLOW);
			
			BYTE* GPrimaryMonitorDeterm = sigscanOffset(textPtr, textPtrEnd,
				//   GetSystemMetrics               GPrimaryMonitorWidth         GPrimaryMonitorHeight    GVirtualScreenRect.left           .top                     .right                 GPrimaryMonitorWorkRect       .bottom     SystemParametersInfoW  mysteryFunc
				"8b 35 ?? ?? ?? ?? 6a 00 ff d6 6a 01 a3 ?? ?? ?? ?? ff d6 6a 4c a3 ?? ?? ?? ?? ff d6 6a 4d a3 ?? ?? ?? ?? ff d6 6a 4e a3 ?? ?? ?? ?? ff d6 6a 4f a3 ?? ?? ?? ?? ff d6 6a 00 68 ?? ?? ?? ?? 6a 00 6a 30 a3 ?? ?? ?? ?? ff 15 ?? ?? ?? ?? e8 ?? ?? ?? ??",
				nullptr, "wholeScreenDeterm");
			if (!GPrimaryMonitorDeterm) {
				showError(L"Couldn't find the place where the game records the size of the whole screen.")
				return;
			}
			
			DWORD GPrimaryMonitorWidth = *(DWORD*)(GPrimaryMonitorDeterm + 0xd);
			DWORD GPrimaryMonitorWorkRectLeft = *(DWORD*)(GPrimaryMonitorDeterm + 0x3a);
			
			BYTE* callInstr = GPrimaryMonitorDeterm + 0x4d;
			BYTE* calledFunction = followRelativeCall(callInstr);
			if (calledFunction == moreSectionCode) {
				// already patched
				calledFunction = followRelativeCall(moreSectionCode);
			}
			offset = calculateRelativeOffset(callInstr, moreSectionCode);
			writeAt(callInstr + 1, &offset, 4)
			
			BYTE newCall[5] {
				0xE8, 0x00, 0x00, 0x00, 0x00
			};
			offset = calculateRelativeOffset(moreSectionCode, calledFunction);
			memcpy(newCall + 1, &offset, 4);
			writeAt(moreSectionCode, newCall, 5)
			
			struct HardcodeWriter {
				HANDLE file;
				RelocTable& relocTable;
				std::vector<BYTE*> relocsToDo;
				void writeHardcode(DWORD varVa, DWORD value) {
					BYTE* filePtr = getFilePos();
					relocsToDo.push_back(filePtr + 2);
					relocsToDo.push_back(filePtr + 6);
					BYTE buf[10] {
						0xC7, 0x05,
							0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00
					};
					
					memcpy(buf + 2, &varVa, 4);
					memcpy(buf + 6, &value, 4);
					
					DWORD bytesWritten = 0;
					write(buf, 10)
				}
				void writeRelocs() {
					for (BYTE* ptr : relocsToDo) {
						relocTable.addEntry(file, ptr, IMAGE_REL_BASED_HIGHLOW);
					}
				}
			} hardcodeWriter{file, relocTable};
			
			hardcodeWriter.writeHardcode(GPrimaryMonitorWidth, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);
			hardcodeWriter.writeHardcode(GPrimaryMonitorWidth + 4, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);
			hardcodeWriter.writeHardcode(GPrimaryMonitorWorkRectLeft, monitorInfo.rcWork.left);
			hardcodeWriter.writeHardcode(GPrimaryMonitorWorkRectLeft + 4, monitorInfo.rcWork.top);
			hardcodeWriter.writeHardcode(GPrimaryMonitorWorkRectLeft + 8, monitorInfo.rcWork.right);
			hardcodeWriter.writeHardcode(GPrimaryMonitorWorkRectLeft + 12, monitorInfo.rcWork.bottom);
			hardcodeWriter.writeHardcode(GPrimaryMonitorWorkRectLeft + 12, monitorInfo.rcWork.bottom);
			
			BYTE ret = 0xC3;
			write(&ret, 1)
			
			moreSectionCode = getFilePos();
			BYTE* UpdateWindow = sigscanOffset(textPtr, textPtrEnd,
					"53 8b 1d ?? ?? ?? ?? 56 57 8b f9 8b 47 74 6a f0 50 ff d3 8b 4f 74 6a ec",
				nullptr, "UpdateWindow");
			BYTE oldUpdateWindowCode[7];
			if (!UpdateWindow) {
				UpdateWindow = sigscanOffset(textPtr, textPtrEnd,
					"e9 ?? ?? ?? ?? 90 90 56 57 8b f9 8b 47 74 6a f0 50 ff d3 8b 4f 74 6a ec",
					nullptr, "UpdateWindow");
				if (!UpdateWindow) {
					showError(L"Couldn't find UpdateWindow function.")
					return;
				}
				BYTE* jumpTarget = followRelativeCall(UpdateWindow);
				if (jumpTarget != moreSectionCode) {
					showError(L"UpdateWindow function has already been patched by an unknown patcher.");
					return;
				}
				memcpy(oldUpdateWindowCode, moreSectionCode, 7);
			} else {
				memcpy(oldUpdateWindowCode, UpdateWindow, 7);
			}
			
			{
				
				std::vector<FoundReloc> relocs = relocTable.findRelocsInRegion(UpdateWindow, UpdateWindow + 7);
				
				for (const FoundReloc& foundReloc : relocs) {
					relocTable.removeEntry(file, foundReloc);
				}
				
			}
			
			BYTE newUpdateWindowCode[7] {
				0xE9, 0x00, 0x00, 0x00, 0x00,
				0x90, 0x90
			};
			offset = calculateRelativeOffset(UpdateWindow, moreSectionCode);
			memcpy(newUpdateWindowCode + 1, &offset, 4);
			writeAt(UpdateWindow, newUpdateWindowCode, 7)
			
			BYTE logic[] { 0x83, 0x7C, 0x24, 0x0C, 0x10,  // CMP dword[ESP+0xc],0x10
				0x77, 0x10,  // JA <skip to instructions below>
				0xC7, 0x44, 0x24, 0x0C, 0x00, 0x00, 0x00, 0x00,  // set PosX
				0xC7, 0x44, 0x24, 0x10, 0x00, 0x00, 0x00, 0x00  // set PosY
			};
			memcpy(logic + 11, &monitorInfo.rcMonitor.left, 4);
			memcpy(logic + 19, &monitorInfo.rcMonitor.top, 4);
			writeAt(moreSectionCode, logic, sizeof logic)
			
			filePos = getFilePos();
			write(oldUpdateWindowCode, 7)
			
			offset = calculateRelativeOffset(getFilePos(), UpdateWindow + 7);
			BYTE jmpBack[5] = {
				0xE9, 0x00, 0x00, 0x00, 0x00
			};
			memcpy(jmpBack + 1, &offset, 4);
			write(jmpBack, 5)
			
			moreSectionCode = getFilePos();
			
			relocTable.addEntry(file, filePos + 3, IMAGE_REL_BASED_HIGHLOW);
			
			
			BYTE* deviceCreation = sigscanOffset(textPtr, textPtrEnd,
					"52 57 50 6a 01 53 55 ff d1 3d 68 08 76 88",
				nullptr, "deviceCreation");
			if (!deviceCreation) {
				deviceCreation = sigscanOffset(textPtr, textPtrEnd,
					"e9 ?? ?? ?? ?? 53 55 ff d1 3d 68 08 76 88",
					nullptr, "deviceCreation");
				if (!deviceCreation) {
					showError(L"Couldn't find Direct 3D device creation.");
					return;
				}
			}
			
			BYTE jumpFromDeviceCreation[5] {
				0xE9, 0x00, 0x00, 0x00, 0x00
			};
			offset = calculateRelativeOffset(deviceCreation, moreSectionCode);
			memcpy(jumpFromDeviceCreation + 1, &offset, 4);
			writeAt(deviceCreation, jumpFromDeviceCreation, 5)
			
			BYTE pushes[] {
				0x52,
				0x57,
				0x50,
				0x6a, 0x01,
				0x6a, 0x99,
				0x55,
				0xff, 0xd1,
				0x3d, 0x68, 0x08, 0x76, 0x88,
				0xE9, 0x00, 0x00, 0x00, 0x00
			};
			pushes[6] = (BYTE)dd.iDevNum;
			offset = calculateRelativeOffset(moreSectionCode + 15, deviceCreation + 14);
			memcpy(pushes + 16, &offset, 4);
			writeAt(moreSectionCode, pushes, sizeof pushes)
			
			ret = 0;
			writeAt(newSectionStart + newSectionSize, &ret, 1)
			SetEndOfFile(file);
			
			hardcodeWriter.writeRelocs();
			
			fileCloser.success = true;
			MessageBoxW(mainWindow, L"Patching successful.", L"Info", MB_OK);
			return;
		}
	}
	MessageBoxW(mainWindow, L"Work rect for this device not found. Don't know where to move window.", L"Error", MB_OK);
}

// Finds if GuiltyGearXrd.exe is currently open and returns the ID of its process
DWORD findOpenGgProcess(HWND* outHwnd) {
	*outHwnd = NULL;
	OBF_LOAD(user32);
	
    // this method was chosen because it's much faster than enumerating all windows or all processes and checking their names
    // also it was chosen because Xrd restarts itself upon launch, and the window appears only on the second, true start
    OBF_IMPORT(user32, FindWindowW);
    *outHwnd = (*FindWindowWPtr)(L"LaunchUnrealUWindowsClient", L"Guilty Gear Xrd -REVELATOR-");
    if (!*outHwnd) return NULL;
    DWORD windsProcId = 0;
    OBF_IMPORT(user32, GetWindowThreadProcessId);
    (*GetWindowThreadProcessIdPtr)(*outHwnd, &windsProcId);
    return windsProcId;
}
