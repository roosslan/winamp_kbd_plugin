/*
 * urlctrl.c - Winamp Multimedia Keyboard Control
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
#include "urlctrl.h"
#include <tchar.h>

#define MAX_URL_LENGTH 201

typedef struct tagURLDETAILS {
	TCHAR     szURL[MAX_URL_LENGTH];
	WNDPROC   lpPrevWndFunc;
	HCURSOR   hCursor;
	HFONT     hFont;
	BOOL      bClicking;
} URLDETAILS, *LPURLDETAILS;

LRESULT CALLBACK UrlControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int CreateUrlControl(HWND hwndDialog, UINT uiControlID, TCHAR *szURL) {
	BOOL bResult = FALSE;

	URLDETAILS *details = NULL;
	HWND hwnd = GetDlgItem(hwndDialog, uiControlID);

	if (hwnd != NULL) {
		details = (URLDETAILS *)HeapAlloc(GetProcessHeap(), 0, sizeof(URLDETAILS));
		if (details != NULL) {
			ZeroMemory(details, sizeof(URLDETAILS));
			if (szURL != NULL) {
				lstrcpyn(details->szURL, szURL, MAX_URL_LENGTH);
			} else {
				lstrcpy(details->szURL, _T(""));
			}
			details->lpPrevWndFunc = (WNDPROC)(LONG_PTR)SetWindowLongPtr(hwnd, GWL_WNDPROC, (LONG)(LONG_PTR)UrlControlProc);
			if (details->lpPrevWndFunc != NULL) {
				bResult = TRUE;
				SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)(LONG_PTR)details);
			}
		}
	}

	if ((bResult == FALSE) && (details != NULL)) {
		HeapFree(GetProcessHeap(), 0, details);
	}

	return bResult;
}

static VOID PaintUrlControl(HWND hwnd, URLDETAILS *details) {
	DWORD dwStyle   = GetWindowLong(hwnd, GWL_STYLE);
	DWORD dwDTStyle = DT_SINGLELINE;
	RECT rect;
	PAINTSTRUCT ps;
	HDC hdc;
	HGDIOBJ hPrevObj;
	int iPrevBkMode;
	TCHAR szWindowText[MAX_URL_LENGTH];
	UINT uFormat;

	// Get the text positioning from the control
	uFormat = DT_SINGLELINE;
	if ((dwStyle & SS_CENTER) == SS_CENTER) {
		uFormat |= DT_CENTER;
	}
	if ((dwStyle & SS_RIGHT) == SS_RIGHT) {
		uFormat |= DT_RIGHT;
	}
	if ((dwStyle & SS_CENTERIMAGE) == SS_CENTERIMAGE) {
		uFormat |= DT_VCENTER;
	}

	// Begin painting
	GetClientRect(hwnd, &rect);
	hdc = BeginPaint(hwnd, &ps);

	// Create the font if this is the first time
	if (details->hFont == NULL) {
		LOGFONT logfont;
		HFONT hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		GetObject(hf, sizeof(LOGFONT), &logfont);
		logfont.lfUnderline = TRUE;
		details->hFont = CreateFontIndirect(&logfont);
		DeleteObject(hf);
	}

	// Choose the underlined font, blue colour, transparent background
	hPrevObj = SelectObject(hdc, details->hFont);
	SetTextColor(hdc, RGB(0, 0, 255));
	iPrevBkMode = SetBkMode(hdc, TRANSPARENT);

	// And draw
	GetWindowText(hwnd, szWindowText, MAX_URL_LENGTH);
	DrawText(hdc, szWindowText, -1, &rect, uFormat);

	// Restore the old GDI object & background mode
	SelectObject(hdc, hPrevObj);
	SetBkMode(hdc, iPrevBkMode);

	// Finish painting
	EndPaint(hwnd, &ps);
}

VOID ExecuteUrlControl(HWND hwnd, URLDETAILS *details) {
	TCHAR szWindowText[MAX_URL_LENGTH];
	if (lstrlen(details->szURL) > 0) {
		ShellExecute(NULL, _T("open"), details->szURL, NULL, NULL, SW_SHOWNORMAL);
	} else {
		GetWindowText(hwnd, szWindowText, MAX_URL_LENGTH);
		ShellExecute(NULL, _T("open"), szWindowText, NULL, NULL, SW_SHOWNORMAL);
	}
}


static LRESULT CALLBACK UrlControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	URLDETAILS *details = (URLDETAILS *)(LONG_PTR)GetWindowLongPtr(hwnd, GWL_USERDATA);
	LRESULT result;

	switch(uMsg) {
		case WM_NCDESTROY:
			// Clean up
			if (details->hCursor != NULL) {
				DestroyCursor(details->hCursor);
			}
			if (details->hFont != NULL) {
				DeleteObject(details->hFont);
			}
			HeapFree(GetProcessHeap(), 0, details);
			break;

		case WM_SETCURSOR:
			if (details->hCursor == NULL) {
				details->hCursor = LoadCursor(NULL, IDC_HAND);
				//details->hCursor = LoadCursor(NULL, IDC_WAIT);
			}
			SetCursor(details->hCursor);
			return TRUE;

		case WM_PAINT:
			PaintUrlControl(hwnd, details);
			return TRUE;

		case WM_SETTEXT:
			result = CallWindowProc(details->lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
			InvalidateRect(hwnd, 0, 0);
			return result;

		case WM_LBUTTONDOWN:
			details->bClicking = TRUE;
			break;

		case WM_LBUTTONUP:
			if (details->bClicking == TRUE) {
				details->bClicking = FALSE;
				ExecuteUrlControl(hwnd, details);
			}
			break;

		case WM_NCHITTEST:
			return HTCLIENT; // This allows us to get mouse messages

		default:
			break;
	}


	// Process other messages using the normal windows procedure
	return CallWindowProc(details->lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
}
