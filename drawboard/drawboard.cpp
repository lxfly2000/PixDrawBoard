// drawboard.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "drawboard.h"
#include "resource.h"
#include <string>
#include <queue>
#include <CommCtrl.h>

#define MAX_LOADSTRING 100
#define WM_DRAWPIXEL (WM_USER+2000)

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
HWND hInputDialog = NULL;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DRAWBOARD, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DRAWBOARD));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DRAWBOARD));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DRAWBOARD);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_MENU_OPEN_INPUT, 0), 0);
   return TRUE;
}

std::queue<std::string>insq;

void ExecuteInstructions(HWND hWnd,char* text)
{
	while (text&&*text)
	{
		if (*text >= '0'&&*text <= '9'||*text>='A'&&*text<='Z')
		{
			char*pEndlSpace = text;
			for (; *pEndlSpace; pEndlSpace++)
			{
				if (*pEndlSpace == '\n' || *pEndlSpace == ' ')
					break;
			}
			if (pEndlSpace == NULL)
			{
				insq.push(std::string(text));
				text = pEndlSpace;
			}
			else
			{
				*pEndlSpace = NULL;
				if (*(pEndlSpace - 1) == '\r')
					*(pEndlSpace - 1) = NULL;
				insq.push(std::string(text));
				text = pEndlSpace + 1;
			}
		}
		else
		{
			char *pn = strchr(text, '\n');
			if (pn == NULL)
			{
				insq.push(std::string(text));
				text = pn;
			}
			else
			{
				*pn = NULL;
				if (*(pn - 1) == '\r')
					*(pn - 1) = NULL;
				insq.push(std::string(text));
				text = pn + 1;
			}
		}
	}
	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);
	SendMessage(hWnd, WM_DRAWPIXEL, 0, 0);
}

int scaleFactor = 1;
int sleepTime = 0;

INT_PTR CALLBACK InputDlgProcess(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_SETRANGE32, 1, 32);
		SendMessage(GetDlgItem(hWnd, IDC_SPIN2), UDM_SETRANGE32, 0, 1000);
		SetDlgItemInt(hWnd, IDC_EDIT_SCALE, 1, TRUE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			scaleFactor = GetDlgItemInt(hWnd, IDC_EDIT_SCALE, NULL, TRUE);
			sleepTime = GetDlgItemInt(hWnd, IDC_EDIT_SLEEP, NULL, TRUE);
			int n = GetWindowTextLengthA(GetDlgItem(hWnd, IDC_EDIT_INPUT));
			char *szBuf = (char*)malloc(sizeof(char)*(n + 1));
			GetDlgItemTextA(hWnd, IDC_EDIT_INPUT, szBuf, n + 1);
			szBuf[n] = NULL;
			ExecuteInstructions(GetParent(hWnd), szBuf);
			free(szBuf);
		}
			break;
		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			break;
		}
		break;
	}
	return 0;
}

DWORD currentColor = 0xFFFFFFFF;

COLORREF ColorToColorRef(DWORD c)
{
	return RGB((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

int PosCodeToAxis(LPCSTR str)
{
	if (str[0] >= 'A'&&str[0] <= 'Z')
		return str[0] - 'A';
	return atoi(str) - 1;
}

void DrawPixel(HWND hWnd)
{
	std::string& strInst = insq.front();
	if (strInst[0] >= '0'&&strInst[0] <= '9'||strInst[0]>='A'&&strInst[0]<='Z')//画图指令
	{
		int row1, col1, row2, col2, colorIndex, type = 0;
		char srow1[4], scol1[4], srow2[4], scol2[4];
		for (char c : strInst)
		{
			if (c == '-')
				type++;
		}
		if (type == 1)
		{
			sscanf_s(strInst.c_str(), "%[^.].%[^-]-%d", srow1, ARRAYSIZE(srow1), scol1, ARRAYSIZE(scol1), &colorIndex);
			row2 = row1 = PosCodeToAxis(srow1);
			col2 = col1 = PosCodeToAxis(scol1);
		}
		else
		{
			sscanf_s(strInst.c_str(), "%[^.].%[^-]-%[^.].%[^-]-%d", srow1, ARRAYSIZE(srow1), scol1, ARRAYSIZE(scol1), srow2, ARRAYSIZE(srow2), scol2, ARRAYSIZE(scol2), &colorIndex);
			row1 = PosCodeToAxis(srow1);
			col1 = PosCodeToAxis(scol1);
			row2 = PosCodeToAxis(srow2);
			col2 = PosCodeToAxis(scol2);
		}
		HDC hdc = GetDC(hWnd);
		COLORREF cref = ColorToColorRef(currentColor);
		HBRUSH hbr = CreateSolidBrush(cref);
		RECT rect{ col1*scaleFactor,row1*scaleFactor,(col2 + 1)*scaleFactor,(row2 + 1)*scaleFactor };
		HGDIOBJ hOld=SelectObject(hdc, hbr);
		FillRect(hdc, &rect, hbr);
		SelectObject(hdc, hOld);
		DeleteObject(hbr);
		ReleaseDC(hWnd, hdc);
	}
	else//其他指令
	{
		if (strInst.substr(0, 4) == "颜色")
			sscanf_s(strInst.c_str() + 5, "%08X", &currentColor);
	}
	insq.pop();
	if (insq.size())
		PostMessage(hWnd, WM_DRAWPIXEL, 0, 0);
	if (sleepTime)
		Sleep(sleepTime);
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
			case ID_MENU_OPEN_INPUT:
				hInputDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_INPUT), hWnd, InputDlgProcess);
				ShowWindow(hInputDialog, SW_SHOW);
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
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_DRAWPIXEL:
		DrawPixel(hWnd);
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
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
