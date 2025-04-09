#ifndef __MD5_H__
#define __MD5_H__

#define MD5_VALUE_LEN      16
#define MD5_STRING_LEN     (32+1)

typedef unsigned char MD5[MD5_VALUE_LEN];
typedef char MD5_STR[MD5_STRING_LEN];

void MD5Buffer(void *buffer, size_t size, MD5 digest);

int MD5Test(void);

void MD5Print(MD5 digest);

void MD5String(MD5 digest, MD5_STR str);

#endif
