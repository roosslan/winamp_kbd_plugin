/*
 * urlctrl.h - Winamp Multimedia Keyboard Control
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
#ifndef _URLCTRL_H_
#define _URLCTRL_H_

#include <windows.h>

BOOL CreateUrlControl(HWND hwndDialog, UINT uiControlID, TCHAR *szURL);


#endif /* _URLCTRL_H_ */