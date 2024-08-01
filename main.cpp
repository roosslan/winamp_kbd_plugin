/*
 * main.c - Winamp Multimedia Keyboard Control
 *
 * Winamp Plugin to control playback via multimedia keyboards
 *
 * Copyright (C) 2003-2005, Daniel Hope
 *
 * Daniel Hope <daniel@hope.co.nz>
 *
 * This program is free software and may be modified and
 * distributed under the terms of the GNU Public License.
 *
 */
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <tchar.h>
#include "gen.h"
#include "frontend.h"
#include "shrmem.h"
#include "urlctrl.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

int id_next = 0x9000;
int id_prev = 0x9001;

static int InitialisePlugin();
static void ConfigurePlugin();
static void QuitPlugin();

static UINT uiNotifyMessage = 0;
static WNDPROC lpWndProcOld;
static CRITICAL_SECTION hookLock;

/* May be accessed from different processes */
static HHOOK hHook = NULL;
static HWND notify = NULL;
static HANDLE hMemory = NULL;
static BOOL bRead = FALSE;
/* End special variables */

typedef struct tagHOOKINFO {
	HHOOK hHook;
	HWND hwndNotification;
	// Other info here if needed
} HOOKINFO, *LPHOOKINFO;

winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,
	"Multimedia keyboard control",
	InitialisePlugin,
	ConfigurePlugin,
	QuitPlugin,
};

typedef struct tagKEYSETTINGS {
	BOOL play;
	BOOL stop;
	BOOL prev;
	BOOL next;
	BOOL volup;
	BOOL voldown;
} KEYSETTINGS;

KEYSETTINGS keySettings;

winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
	return &plugin;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	if (message == WM_HOTKEY)
	{
		if (LOWORD(wParam) == id_next)	
			SendMessage(hwnd, WM_COMMAND, 40048, 0); // APPCOMMAND_MEDIA_NEXTTRACK
		if (LOWORD(wParam) == id_prev)
			SendMessage(hwnd, WM_COMMAND, 40044, 0); // APPCOMMAND_MEDIA_PREVIOUSTRACK
	}

	if (message == uiNotifyMessage) {
		int status = (int)SendMessage(hwnd, WM_WA_IPC, 0, 104);
		int offset;
		switch (lParam) {
			case VK_MEDIA_NEXT_TRACK:
				SendMessage(hwnd, WM_COMMAND, WINAMP_BUTTON5, 0); // Next
				break;

			case VK_MEDIA_PREV_TRACK:
				if (status == 0) {
					SendMessage(hwnd, WM_COMMAND, WINAMP_BUTTON1, 0); // Prev
				} else {
					offset = (int)SendMessage(hwnd, WM_WA_IPC, 0, 105);
					if ((status != 3) && (offset > 2000)) {
						SendMessage(hwnd, WM_COMMAND, WINAMP_BUTTON2, 0); // Play
					} else {
						SendMessage(hwnd, WM_COMMAND, WINAMP_BUTTON1, 0); // Prev
					}
				}
				break;

			case VK_MEDIA_STOP:
				SendMessage(hwnd, WM_COMMAND, WINAMP_BUTTON4, 0); // Stop
				break;

			case VK_MEDIA_PLAY_PAUSE:
				if (status != 1) {
					SendMessage(hwnd, WM_COMMAND, WINAMP_BUTTON2, 0); // Play
				} else {
					SendMessage(hwnd, WM_COMMAND, WINAMP_BUTTON3, 0); // Pause
				}
				break;

			case VK_VOLUME_DOWN:
				SendMessage(hwnd, WM_COMMAND, WINAMP_VOLUMEDOWN, 0); // Volume down
				break;

			case VK_VOLUME_UP:
				SendMessage(hwnd, WM_COMMAND, WINAMP_VOLUMEUP, 0); // Volume up
				break;

			default:
				break;
		}
		return 0;
	}
	return CallWindowProc(lpWndProcOld, hwnd, message, wParam, lParam);
}

BOOL IsValidKey(DWORD code) {
	switch (code) {
		case VK_MEDIA_NEXT_TRACK:
			return keySettings.next;
		case VK_MEDIA_PREV_TRACK:
			return keySettings.prev;
		case VK_MEDIA_STOP:
			return keySettings.stop;
		case VK_MEDIA_PLAY_PAUSE:
			return keySettings.play;
		case VK_VOLUME_DOWN:
			return keySettings.voldown;
		case VK_VOLUME_UP:
			return keySettings.volup;
		default:
			break;
	}
	return FALSE;
}

LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	// Try to read the hook infomation from our shared memory segment
	if (bRead == FALSE) {
		HANDLE hMemory = OpenMemoryHandle();
		if (hMemory != NULL) {
			HOOKINFO info;
			if (ReadMemory(hMemory, (LPBYTE)&info, sizeof(HOOKINFO))) {
				hHook = info.hHook;
				notify = info.hwndNotification;
				uiNotifyMessage = RegisterWindowMessage(_T("wakeybdnotify"));
			}
			CloseMemoryHandle(hMemory);
		}
		bRead = TRUE;
	}

	if ((nCode >= 0) && (notify != NULL) && (uiNotifyMessage != 0)) {

		KBDLLHOOKSTRUCT *key = (KBDLLHOOKSTRUCT *)lParam;

		switch (key->vkCode) {
			case VK_MEDIA_NEXT_TRACK:
			case VK_MEDIA_PREV_TRACK:
			case VK_MEDIA_STOP:
			case VK_MEDIA_PLAY_PAUSE:
			case VK_VOLUME_DOWN:
			case VK_VOLUME_UP:
				if (IsValidKey(key->vkCode)) {
					if (wParam == WM_KEYDOWN) {
						PostMessage(notify, uiNotifyMessage, 0, key->vkCode);
					}
					return -1;
				}
				break;

			default:
				break;
		}
	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

BOOL InstallHook() {
	// This function is called in the context of the original process (the hooker)
	BOOL bResult = FALSE;

	EnterCriticalSection(&hookLock);

	if (hHook == NULL) {

		// Set the hHook within our process context
		hHook = SetWindowsHookEx(WH_KEYBOARD_LL, HookProc, plugin.hDllInstance, 0);

		if (hHook != NULL) {

			hMemory = CreateMemoryHandle();
			if (hMemory != NULL) {
				HOOKINFO info;
				info.hHook = hHook;
				info.hwndNotification = plugin.hwndParent;

				// Tell the target process about it too
				WriteMemory(hMemory, (LPBYTE)&info, sizeof(HOOKINFO));
			}
			notify = plugin.hwndParent;
			bResult = TRUE;
		}
	}

	LeaveCriticalSection(&hookLock);

	return bResult;
}

BOOL UninstallHook() {
	// This function is called in the context of the original process (the hooker)
	BOOL bResult = FALSE;

	EnterCriticalSection(&hookLock);

	if (hHook != NULL) {
		if (UnhookWindowsHookEx(hHook)) {
			bResult = TRUE;
		}

		if (hMemory != NULL) {
			CloseMemoryHandle(hMemory);
		}

	} else {
		bResult = TRUE;
	}

	LeaveCriticalSection(&hookLock);

	return bResult;
}

const TCHAR *SETTINGS_KEY = _T("SOFTWARE\\alabuga.dev\\Winamp keyboard control");

BOOL ReadBool(HKEY hKey, TCHAR *value, BOOL bDefault) {
	BOOL result = bDefault;
	DWORD read;
	DWORD size = sizeof(read);
	if (RegQueryValueEx(hKey,
		value,
		NULL,
		NULL,
		(LPBYTE)&read,
		&size) == ERROR_SUCCESS) {

		result = (read == 0) ? FALSE : TRUE;
	}
	return result;
}

void WriteBool(HKEY hKey, TCHAR *value, BOOL bValue) {
	DWORD val = (bValue == TRUE) ? 1 : 0;
	RegSetValueEx(hKey, value, 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
}

BOOL OpenKey(HKEY *hKey) {
	return (RegCreateKeyEx(HKEY_CURRENT_USER,
		SETTINGS_KEY,
		0,
		NULL,
		0,
		KEY_ALL_ACCESS,
		NULL,
		hKey,
		NULL) == ERROR_SUCCESS) ? TRUE : FALSE;
}

void LoadKeySettings() {
	HKEY hKey;
	if (OpenKey(&hKey)) {
		keySettings.play = ReadBool(hKey, _T("Key_Play"), TRUE);
		keySettings.stop = ReadBool(hKey, _T("Key_Stop"), TRUE);
		keySettings.prev = ReadBool(hKey, _T("Key_Prev"), TRUE);
		keySettings.next = ReadBool(hKey, _T("Key_Next"), TRUE);
		keySettings.volup = ReadBool(hKey, _T("Key_VolUp"), FALSE);
		keySettings.voldown = ReadBool(hKey, _T("Key_VolDown"), FALSE);
		RegCloseKey(hKey);
	}
}

void SaveKeySettings() {
	HKEY hKey;
	if (OpenKey(&hKey)) {
		WriteBool(hKey, _T("Key_Play"), keySettings.play);
		WriteBool(hKey, _T("Key_Stop"), keySettings.stop);
		WriteBool(hKey, _T("Key_Prev"), keySettings.prev);
		WriteBool(hKey, _T("Key_Next"), keySettings.next);
		WriteBool(hKey, _T("Key_VolUp"), keySettings.volup);
		WriteBool(hKey, _T("Key_VolDown"), keySettings.voldown);
		RegCloseKey(hKey);
	}
}

int InitialisePlugin() {

	RegisterHotKey(plugin.hwndParent, id_next, MOD_ALT | MOD_CONTROL, VK_NEXT);
	RegisterHotKey(plugin.hwndParent, id_prev, MOD_ALT | MOD_CONTROL, VK_PRIOR);

	LoadKeySettings();
	uiNotifyMessage = RegisterWindowMessage(_T("wakeybdnotify"));
	lpWndProcOld = (WNDPROC)(LONG_PTR)SetWindowLong(plugin.hwndParent, GWL_WNDPROC, (LONG)(LONG_PTR)WndProc);
	InitializeCriticalSection(&hookLock);
	InstallHook();
	return 0;
}

INT_PTR CALLBACK ConfigureProcAbout(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			CreateUrlControl(hwnd, IDC_HOMEPAGE, _T("https://www.alabuga.dev/"));
			break;
		default:
			break;
	}
	return FALSE;
}

INT_PTR CALLBACK ConfigureProcSettings(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LPNMHDR nmhdr;
	switch (uMsg) {
		case WM_INITDIALOG:
			CheckDlgButton(hwnd, IDC_PG, BST_CHECKED);
			CheckDlgButton(hwnd, IDC_PLAY, keySettings.play ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_STOP, keySettings.stop ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_PREV, keySettings.prev ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_NEXT, keySettings.next ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_VOLUP, keySettings.volup ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_VOLDOWN, keySettings.voldown ? BST_CHECKED : BST_UNCHECKED);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_PLAY:
				case IDC_STOP:
				case IDC_PREV:
				case IDC_NEXT:
				case IDC_VOLUP:
				case IDC_VOLDOWN:
					PropSheet_Changed(GetParent(hwnd), hwnd);
					break;
			}
			break;
		case WM_NOTIFY:
			nmhdr = (LPNMHDR)lParam;
			switch (nmhdr->code) {
				case PSN_APPLY:
					keySettings.play = IsDlgButtonChecked(hwnd, IDC_PLAY) ? TRUE : FALSE;
					keySettings.stop = IsDlgButtonChecked(hwnd, IDC_STOP) ? TRUE : FALSE;
					keySettings.prev = IsDlgButtonChecked(hwnd, IDC_PREV) ? TRUE : FALSE;
					keySettings.next = IsDlgButtonChecked(hwnd, IDC_NEXT) ? TRUE : FALSE;
					keySettings.volup = IsDlgButtonChecked(hwnd, IDC_VOLUP) ? TRUE : FALSE;
					keySettings.voldown = IsDlgButtonChecked(hwnd, IDC_VOLDOWN) ? TRUE : FALSE;
					SaveKeySettings();
					return PSNRET_NOERROR;
					break;
				default:
					break;
			}
		default:
			break;
	}
	return FALSE;
}

void ConfigurePlugin() {
	UINT nPages = 2;
	PROPSHEETPAGE pages[2];
	
	ZeroMemory(pages+0, nPages * sizeof(PROPSHEETPAGE));

	pages[0].dwSize = sizeof(PROPSHEETPAGE);
	pages[0].dwFlags = PSP_DEFAULT;
	pages[0].hInstance = plugin.hDllInstance;
	pages[0].pszTemplate = MAKEINTRESOURCE(IDD_CONFIGURE2);
	pages[0].pszTitle = _T("Settings");
	pages[0].pfnDlgProc = (DLGPROC)ConfigureProcSettings;

	pages[1].dwSize = sizeof(PROPSHEETPAGE);
	pages[1].dwFlags = PSP_DEFAULT;
	pages[1].hInstance = plugin.hDllInstance;
	pages[1].pszTemplate = MAKEINTRESOURCE(IDD_ABOUT);
	pages[1].pszTitle = _T("About");
	pages[1].pfnDlgProc = (DLGPROC)ConfigureProcAbout;

	PROPSHEETHEADER psh;
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_DEFAULT | PSH_NOCONTEXTHELP | PSH_PROPSHEETPAGE;
	psh.hwndParent = plugin.hwndParent;
	psh.hInstance = plugin.hDllInstance;
	psh.pszCaption = _T("Winamp multimedia keyboard control");
	psh.nPages = nPages;
	psh.ppsp = (PROPSHEETPAGE *)&pages;

	PropertySheet(&psh);
}

void QuitPlugin() {
	UnregisterHotKey(plugin.hwndParent, id_next);
	UnregisterHotKey(plugin.hwndParent, id_prev);
	UninstallHook();
	DeleteCriticalSection(&hookLock);
}
