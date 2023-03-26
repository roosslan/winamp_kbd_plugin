#ifndef _GEN_H_
#define _GEN_H_

#include <windows.h>

typedef struct {
	int version;
	char *description;
	int (*init)();
	void (*config)();
	void (*quit)();
	HWND hwndParent;
	HINSTANCE hDllInstance;
} winampGeneralPurposePlugin;

#define GPPHDR_VER 0x10

extern winampGeneralPurposePlugin *gen_plugins[256];
typedef winampGeneralPurposePlugin * (*winampGeneralPurposePluginGetter)();

#endif /* _GEN_H_ */