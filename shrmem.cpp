/*
 * shrmem.c - Winamp Multimedia Keyboard Control
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
#include "shrmem.h"

#include <tchar.h>
const TCHAR *MEMORY_FILENAME = _T("{E75274E5-CAAD-4a4f-9CE0-EAD3146D3810}");
const TCHAR *MEMORY_MUTEX    = _T("{E75274E5-CAAD-4a4f-9CE0-EAD3146D3810}");


VOID Lock();
VOID Unlock();

HANDLE CreateMemoryHandle() {
	HANDLE result = NULL;
//	__try {
		result = CreateFileMapping(INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			64,
			MEMORY_FILENAME);
//	}
//	__except(EXCEPTION_EXECUTE_HANDLER) {
//		result = NULL;
//	}
	return result;
}

HANDLE OpenMemoryHandle() {
	HANDLE result = NULL;

//	__try {
		result = OpenFileMapping(
			FILE_MAP_READ | FILE_MAP_WRITE,
			FALSE,
			MEMORY_FILENAME);
//	}
//	__except(EXCEPTION_EXECUTE_HANDLER) {
//		result = NULL;
//	}

	return result;
}

VOID CloseMemoryHandle(HANDLE hMemory) {
	if (hMemory != NULL) {
//		__try {
			CloseHandle(hMemory);
//		}
//		__except(EXCEPTION_EXECUTE_HANDLER) {
//		}
	}
}

BOOL WriteMemory(HANDLE hMemory, const LPBYTE lpBuffer, const DWORD dwBufferSize) {
	BOOL result = FALSE;
	LPVOID lpAddress = NULL;

	if (hMemory != NULL) {
//		__try {
			Lock();

			lpAddress = MapViewOfFile(
				hMemory,
				FILE_MAP_READ | FILE_MAP_WRITE,
				0,
				0,
				0);

			if (lpAddress != NULL) {
				CopyMemory(lpAddress, lpBuffer, dwBufferSize);
				result = TRUE;
			}

//		}
//		__except(EXCEPTION_EXECUTE_HANDLER) {
//			result = FALSE;
//		}

		// Unmap the file (if it was mapped)
//		__try {
			if (lpAddress != NULL) {
				UnmapViewOfFile(lpAddress);
			}
//		}
//		__except(EXCEPTION_EXECUTE_HANDLER) {
//		}

		Unlock();
	}

	return result;
}

BOOL ReadMemory(HANDLE hMemory, LPBYTE lpBuffer, DWORD dwBufferSize) {
	BOOL result = FALSE;
	LPVOID lpAddress = NULL;

	if (hMemory != NULL) {
//		__try {
			Lock();

			lpAddress = MapViewOfFile(
				hMemory,
				FILE_MAP_READ | FILE_MAP_WRITE,
				0,
				0,
				0);

			if (lpAddress != NULL) {
				CopyMemory(lpBuffer, lpAddress, dwBufferSize);
				result = TRUE;
			}

//		}
//		__except(EXCEPTION_EXECUTE_HANDLER) {
//			result = FALSE;
//		}

		// Unmap the file (if it was mapped)
//		__try {
			if (lpAddress != NULL) {
				UnmapViewOfFile(lpAddress);
			}
//		}
//		__except(EXCEPTION_EXECUTE_HANDLER) {
//		}

		Unlock();
	}

	return result;
}

VOID Lock() {
	// TODO:
}

VOID Unlock() {
	// TODO:
}