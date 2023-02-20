#include <Windows.h>
#include <CommCtrl.h>
#include <WindowsX.h>
#include <Shlwapi.h>
#include <vector>
#include <string>
#include "resource.h"

#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"comctl32.lib")

#define ID_TIMER_WAIT 1

std::vector<std::wstring>qMessages;
std::vector<int>qMsgStartPos;
int currentLine = 0;
int waitTime, sleepTime;
WORD hotkeyOnoff;
HHOOK hHook = NULL;

BOOL SetClipBoardText(HWND hWnd, LPCTSTR text)
{
	if (FAILED(OpenClipboard(hWnd)))
		return FALSE;
	EmptyClipboard();
	HGLOBAL hClip = GlobalAlloc(GMEM_MOVEABLE, (lstrlen(text) + 1) * sizeof(TCHAR));//申请剪贴板内存
	TCHAR *pBuffer = (TCHAR*)GlobalLock(hClip);//获取剪贴板的地址并锁定。
	StrCpy(pBuffer, text);
	GlobalUnlock(hClip);//释放锁
	SetClipboardData(CF_UNICODETEXT, hClip);
	CloseClipboard();
	return TRUE;
}

void ReadInput(HWND hWnd)
{
	qMessages.clear();
	qMsgStartPos.clear();
	int nLength = GetWindowTextLength(GetDlgItem(hWnd, IDC_EDIT_INPUT));
	TCHAR *szBuf = (TCHAR*)malloc(sizeof(TCHAR)*(nLength + 1));
	GetDlgItemText(hWnd, IDC_EDIT_INPUT, szBuf, nLength + 1);
	szBuf[nLength] = '\0';
	for (TCHAR*p = szBuf; p&&*p;)
	{
		TCHAR*pEndl = wcschr(p, '\n');
		if (pEndl == nullptr)
		{
			qMessages.push_back(std::wstring(p));
			qMsgStartPos.push_back(p - szBuf);
			p = pEndl;
		}
		else
		{
			if (*(pEndl - 1) == '\r')
				*(pEndl - 1) = '\0';
			*pEndl = '\0';
			qMessages.push_back(std::wstring(p));
			qMsgStartPos.push_back(p - szBuf);
			p = pEndl + 1;
		}
	}
	free(szBuf);
	if (currentLine >= qMessages.size())
		SetDlgItemInt(hWnd, IDC_EDIT_LINE_NUMBER_TO_SEND, currentLine = 0, TRUE);
}

bool isRunning = false;

void OnCheckOnOff(HWND hWnd,bool on)
{
	if (on)
	{
		waitTime = GetDlgItemInt(hWnd, IDC_EDIT_WAIT_TIME, NULL, TRUE);
		sleepTime = GetDlgItemInt(hWnd, IDC_EDIT_SLEEP_TIME, NULL, TRUE);
		ReadInput(hWnd);
		if (qMessages.size())
		{
			SetDlgItemText(hWnd, IDC_EDIT_MSG_TO_SEND, qMessages[0].c_str());
			Edit_SetSel(GetDlgItem(hWnd, IDC_EDIT_INPUT), qMsgStartPos[0], qMsgStartPos.size() > 1 ? qMsgStartPos[1] : -1);
		}
		SetTimer(hWnd, ID_TIMER_WAIT, waitTime, NULL);
	}
	else
	{
		KillTimer(hWnd, ID_TIMER_WAIT);
	}
	isRunning = on;
}

void OnTimerCopy(HWND hWnd)
{
	//复制
	Sleep(sleepTime);
	if (!SetClipBoardText(hWnd, qMessages[currentLine].c_str()))
	{
		MessageBeep(MB_ICONERROR);
		SetDlgItemText(hWnd, IDC_EDIT_STATUS, TEXT("复制到剪贴板失败。"));
		return;
	}
	SetDlgItemText(hWnd, IDC_EDIT_STATUS, TEXT("复制到剪贴板成功。"));
	//粘贴
	Sleep(sleepTime);
	keybd_event(VK_LCONTROL, 0, NULL, NULL);
	keybd_event('V', 0, NULL, NULL);
	Sleep(sleepTime);
	keybd_event('V', 0, KEYEVENTF_KEYUP, NULL);
	keybd_event(VK_LCONTROL, 0, KEYEVENTF_KEYUP, NULL);
	SetDlgItemText(hWnd, IDC_EDIT_STATUS, TEXT("已按下Ctrl-V."));
	//发送
	Sleep(sleepTime);
	keybd_event(VK_RETURN, 0, NULL, NULL);
	Sleep(sleepTime);
	keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, NULL);
	SetDlgItemText(hWnd, IDC_EDIT_STATUS, TEXT("已按下Enter."));
	currentLine++;
	if (IsDlgButtonChecked(hWnd, IDC_CHECK_CYCLING) == BST_CHECKED && currentLine >= qMessages.size())
		currentLine = 0;
	SetDlgItemInt(hWnd, IDC_EDIT_LINE_NUMBER_TO_SEND, currentLine, TRUE);
	if (currentLine >= qMessages.size())
	{
		CheckDlgButton(hWnd, IDC_CHECK_ONOFF, BST_UNCHECKED);
		OnCheckOnOff(hWnd, false);
	}
}

BOOL IsKeyPress(DWORD vk, WORD vc)
{
	if (HIBYTE(vc) & HOTKEYF_ALT)
	{
		if ((GetAsyncKeyState(VK_MENU) & 0x8000) == 0)
			return FALSE;
	}
	if (HIBYTE(vc) & HOTKEYF_SHIFT)
	{
		if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0)
			return FALSE;
	}
	if (HIBYTE(vc) & HOTKEYF_CONTROL)
	{
		if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0)
			return FALSE;
	}
	return LOBYTE(vc) == (vk & 0xFF);
}

HWND hMainDlg;

LRESULT CALLBACK HookProcess(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		switch (wParam)
		{
		case WM_KEYUP:case WM_SYSKEYUP:
		{
			PKBDLLHOOKSTRUCT pk = (PKBDLLHOOKSTRUCT)lParam;
			if (IsKeyPress(pk->vkCode, hotkeyOnoff) && GetFocus() != GetDlgItem(hMainDlg, IDC_HOTKEY_ONOFF))
			{
				bool on = IsDlgButtonChecked(hMainDlg, IDC_CHECK_ONOFF) == BST_CHECKED ? false : true;
				CheckDlgButton(hMainDlg, IDC_CHECK_ONOFF, on ? BST_CHECKED : BST_UNCHECKED);
				OnCheckOnOff(hMainDlg, on);
			}
		}
			break;
		}
	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void DlgInit(HWND hWnd)
{
	SendMessage(GetDlgItem(hWnd, IDC_SPIN_WAIT_TIME), UDM_SETRANGE32, 0, 100000);
	SendMessage(GetDlgItem(hWnd, IDC_SPIN_SLEEP_TIME), UDM_SETRANGE32, 0, 100000);
	SendMessage(GetDlgItem(hWnd, IDC_SPIN_LINE_NUMBER_TO_SEND), UDM_SETRANGE32, 0, 100000);
	SetDlgItemInt(hWnd, IDC_EDIT_WAIT_TIME, 6000, TRUE);
	SetDlgItemInt(hWnd, IDC_EDIT_SLEEP_TIME, 100, TRUE);
	SendMessage(GetDlgItem(hWnd, IDC_HOTKEY_ONOFF), HKM_SETHOTKEY, (WPARAM)VK_F2, 0);
	hotkeyOnoff = (WORD)SendMessage(GetDlgItem(hWnd, IDC_HOTKEY_ONOFF), HKM_GETHOTKEY, 0, 0);
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, HookProcess, GetModuleHandle(NULL), NULL);
	hMainDlg = hWnd;
}

void OnCancel(HWND hWnd)
{
	if (hHook)
		UnhookWindowsHookEx(hHook);
	PostQuitMessage(0);
}

INT_PTR CALLBACK DlgProcess(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:DlgInit(hWnd);break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:OnCancel(hWnd); break;
		case IDC_CHECK_ONOFF:OnCheckOnOff(hWnd, IsDlgButtonChecked(hWnd, IDC_CHECK_ONOFF) == BST_CHECKED); break;
		case IDC_EDIT_LINE_NUMBER_TO_SEND:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				currentLine = GetDlgItemInt(hWnd, IDC_EDIT_LINE_NUMBER_TO_SEND, NULL, TRUE);
				if (currentLine >= qMessages.size())
				{
					SetDlgItemText(hWnd, IDC_EDIT_MSG_TO_SEND, TEXT("【已到末尾】"));
					if (isRunning)
						Edit_SetSel(GetDlgItem(hWnd, IDC_EDIT_INPUT), -1, -1);
				}
				else
				{
					SetDlgItemText(hWnd, IDC_EDIT_MSG_TO_SEND, qMessages[currentLine].c_str());
					HWND hBox = GetDlgItem(hWnd, IDC_EDIT_INPUT);
					if (isRunning)
					{
						Edit_SetSel(hBox, qMsgStartPos[currentLine], qMsgStartPos.size() > currentLine + 1 ? qMsgStartPos[currentLine + 1] : -1);
						Edit_ScrollCaret(hBox);
					}
				}
			}
			break;
		case IDC_EDIT_INPUT:
			if (HIWORD(wParam) == EN_CHANGE)
				SetDlgItemInt(hWnd, IDC_EDIT_LINE_NUMBER_TO_SEND, currentLine = 0, TRUE);
			break;
		case IDC_HOTKEY_ONOFF:
			hotkeyOnoff = (WORD)SendMessage(GetDlgItem(hWnd, IDC_HOTKEY_ONOFF), HKM_GETHOTKEY, 0, 0);
			break;
		}
		break;
	case WM_TIMER:
		switch (wParam)
		{
		case ID_TIMER_WAIT:OnTimerCopy(hWnd);break;
		}
		break;
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hInst,HINSTANCE hPrevInst,LPWSTR param,int iShow)
{
	InitCommonControls();
	return DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_AUTOSEND), NULL, DlgProcess);
}