typedef    unsigned char        byte;
typedef long	DWORD;
unsigned int  *seedToKey_bosch_cng_ota(byte *seed, unsigned int length, byte *key,unsigned int retLen);
unsigned int  *seedToKey_bosch_cng_app(byte *seed, unsigned int length, byte *key,unsigned int retLen);