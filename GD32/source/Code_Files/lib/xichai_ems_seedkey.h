typedef long	DWORD;
typedef unsigned char uchar;
#define TRUE 1
DWORD seedToKey(uchar *seed, DWORD length, uchar *key, DWORD *retLen);
