/*
 * File: compress.h
 * Purpose: To compress file using the Haffman algorithm
 * Author: puresky
 * Date: 2011/05/01
 */

#ifndef _FILE_COMPRESSION_H
#define _FILE_COMPRESSION_H


//Haffuman Tree Node
typedef struct HaffumanTreeNode HTN;
struct HaffumanTreeNode
{
      char _ch;   //character
      int _count; //frequency
      struct HaffumanTreeNode *_left; //left child
      struct HaffumanTreeNode *_right;//rigth child
};

//FileCompress Struct
#define BITS_PER_CHAR 8     //the number of bits in a char
#define MAX_CHARS 256            //the max number of chars
#define FILE_BUF_SIZE 4096  //the size of Buffer for FILE I/O
typedef struct FileCompressStruct FCS;
struct FileCompressStruct
{
      HTN *_haffuman;        //A pointer to the root of hafumman tree
      unsigned int _charsCount; //To store the number of chars
      unsigned int _total; //Total bytes in a file.
      char *_dictionary[MAX_CHARS]; //to store the encoding of each character
      int _statistic[MAX_CHARS]; //To store the number of each character
};

typedef struct AtbmEncryptedStruct AES;
struct AtbmEncryptedStruct
{
	unsigned int _charsCount;
	int _statistic[MAX_CHARS];
	int buf_len;
	unsigned char buf[];
};

typedef void (*DECOPRESS_CALLBACK)(void *para, unsigned int *addr, int offset, int size);

struct CallBackPara{
	void *priv;
	unsigned int addr;
	DECOPRESS_CALLBACK cb;
};
typedef struct CallBackPara CBP;

FCS *fcs_new();
void fcs_compress(FCS *fcs, const char *inFileName, const char *outFileName);
void fcs_decompress(AES *aes, CBP *cbp);
void fcs_free(FCS *fcs);
#endif