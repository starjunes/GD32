//typedef unsigned int DWORD;
//typedef unsigned char uchar;
#include "xichai_ems_seedkey.h"
DWORD seedToKey_EControl_app(uchar *seed,DWORD length,uchar *key,DWORD*retlen);
DWORD seedToKey_EControl_ota(uchar *seed, DWORD length, uchar *key, DWORD *retLen);