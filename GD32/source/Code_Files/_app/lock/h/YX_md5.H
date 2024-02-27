#include  "yx_includes.h"
#ifndef _H_CAN_MD5
#define _H_CAN_MD5

/* POINTER defines a generic pointer type */  
typedef INT8U * POINTER;  
  
/* UINT2 defines a two byte word */  
//typedef unsigned short int UINT2;   
  
  
  
/* MD5 context. */  
typedef struct {  
    INT32U state[4];                                   /* state (ABCD) */  
    INT32U count[2];        /* number of bits, modulo 2^64 (lsb first) */  
    INT8U  buffer[64];                         /* input buffer */  
} MD5_CTX;  
  
void MD5Init (MD5_CTX *context);  
void MD5Update (MD5_CTX *context, INT8U *input, INT32U inputLen);  
//void MD5UpdaterString(MD5_CTX *context,const char *string);  
//int MD5FileUpdateFile (MD5_CTX *context,char *filename);  
void MD5Final (INT8U *digest, MD5_CTX *context);  
void MDString(INT8U *string,INT8U *digest,INT8U dlen) ;
//int MD5File (char *filename,unsigned char digest[16]);  
#endif
