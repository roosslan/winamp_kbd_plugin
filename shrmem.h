/*
 * shrmem.h - Winamp Multimedia Keyboard Control
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
#ifndef _SHRMEM_H_
#define _SHRMEM_H_

#include <windows.h>

HANDLE create_memory_handle();
HANDLE open_memory_handle();
VOID close_memory_handle(HANDLE hMemory);
BOOL write_memory(HANDLE hMemory, const LPBYTE lpBuffer, const DWORD dwBufferSize);
BOOL read_memory(HANDLE hMemory, LPBYTE lpBuffer, DWORD dwBufferSize);

#endif /* _SHRMEM_H_ */