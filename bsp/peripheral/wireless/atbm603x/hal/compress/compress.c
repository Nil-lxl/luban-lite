/*
 * File: compress.c
 * Purpose: To compress file using the Haffman algorithm
 * Author: puresky
 * Date: 2011/05/01
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "compress.h"
#include "pq.h"

#define SUPPORT_COMPRESS 0
static const unsigned char mask[8] =
{
      0x80, /* 10000000 */
      0x40, /* 01000000 */
      0x20, /* 00100000 */
      0x10, /* 00010000 */
      0x08, /* 00001000 */
      0x04, /* 00000100 */
      0x02, /* 00000010 */
      0x01  /* 00000001 */                       
};


//static functions of HTN
static HTN *htn_new(char ch, int count)
{
      HTN *htn = (HTN *)malloc(sizeof(HTN));
      htn->_left = NULL;
      htn->_right = NULL;
      htn->_ch = ch;
      htn->_count = count;
      return htn;
}

static void htn_print_recursive(HTN *htn, int depth)
{
      int i;
      if(htn)
      {
            for(i = 0; i < depth; ++i)
                  printf("  ");
            printf("%d:%d\n", htn->_ch, htn->_count);
            htn_print_recursive(htn->_left, depth + 1);
            htn_print_recursive(htn->_right, depth + 1);
      }

}

static void htn_print(HTN *htn)
{
      htn_print_recursive(htn, 0);
}

static void htn_free(HTN *htn)
{
      if(htn)
      {
            htn_free(htn->_left);
            htn_free(htn->_right);
            free(htn);
      }
}

 

//static functions of FCS
#if SUPPORT_COMPRESS
static void fcs_generate_statistic(FCS *fcs, const char *inFileName)
{
      int ret, i;
      unsigned char buf[FILE_BUF_SIZE];
      FILE *pf = fopen(inFileName, "rb");
      if(!pf)
      {
            fprintf(stderr, "can't open file:%s\n", inFileName);
            return;
      }

      while((ret = fread(buf, 1, FILE_BUF_SIZE, pf)) > 0)
      {
            fcs->_total += ret;
            for(i = 0; i < ret; ++i)
            {
                  if(fcs->_statistic[buf[i]] == 0)
                        fcs->_charsCount++;
                  fcs->_statistic[buf[i]]++;
            }
      }
      fclose(pf);
}
#endif

static void fcs_create_haffuman_tree(FCS *fcs)
{
      int i, count;
      HTN *htn, *parent, *left, *right;
      KeyValue *kv, *kv1, *kv2;
      PriorityQueue *pq;
      pq = priority_queue_new(PRIORITY_MIN);

      for(i = 0; i < MAX_CHARS; ++i)
      {
            if(fcs->_statistic[i])
            {
                  htn = htn_new((char)i, fcs->_statistic[i]);
                  kv = key_value_new(fcs->_statistic[i], htn);
                  priority_queue_enqueue(pq, kv);
            }
      }
      //fprintf(stdout, "the number of haffuman leaf is %d\n", priority_queue_size(pq));
     
      while(!priority_queue_empty(pq))
      {
            //fprintf(stdout, "priority queue size:%d\n", priority_queue_size(pq));
            kv1 = priority_queue_dequeue(pq);
            kv2 = priority_queue_dequeue(pq);
            if(kv2 == NULL)
            {
                  fcs->_haffuman = kv1->_value;
                  key_value_free(kv1, NULL);
            }
            else
            {
                  left = (HTN *)kv1->_value;
                  right = (HTN *)kv2->_value;
                  count = left->_count + right->_count;
                  key_value_free(kv1, NULL);
                  key_value_free(kv2, NULL);
                  parent = htn_new(0, count);
                  parent->_left = left;
                  parent->_right = right;
                  kv = key_value_new(count, parent);
                  priority_queue_enqueue(pq, kv);
            }
      }

      priority_queue_free(pq, NULL);

      //htn_print(fcs->_haffuman);
}

static void fcs_generate_dictionary_recursively(HTN *htn, char *dictionary[], char path[], int depth)
{
      char *code = NULL;
      if(htn)
      {
            if(htn->_left == NULL && htn->_right == NULL)
            {
                  code = (char *)malloc(sizeof(char) * (depth + 1));
                  atbm_memset(code, 0, sizeof(char) * (depth + 1));
                  memcpy(code, path, depth);
                  dictionary[(unsigned char)htn->_ch] = code;
            }

            if(htn->_left)
            {
                  path[depth] = '0';
                  fcs_generate_dictionary_recursively(htn->_left, dictionary, path, depth + 1);
            }

            if(htn->_right)
            {
                  path[depth] = '1';
                  fcs_generate_dictionary_recursively(htn->_right, dictionary, path, depth + 1);
            }
      }
}

static void fcs_generate_dictionary(FCS *fcs)
{
      char path[32];
      fcs_generate_dictionary_recursively(fcs->_haffuman, fcs->_dictionary, path, 0);
      //fcs_print_dictionary(fcs);
}

static void fcs_print_dictionary(FCS *fcs)
{
      int i;
      for(i = 0; i < MAX_CHARS; ++i)
            if(fcs->_dictionary[i] != NULL)
                  fprintf(stdout, "%d:%s\n", i, fcs->_dictionary[i]);
}

#if SUPPORT_COMPRESS
static void fcs_write_statistic(FCS *fcs, FILE *pf)
{
      int i;
      fprintf(pf, "%d\n", fcs->_charsCount);
      for(i = 0; i < MAX_CHARS; ++i)
            if(fcs->_statistic[i] != 0)
                  fprintf(pf, "%d %d\n", i, fcs->_statistic[i]);
}

static void fcs_do_compress(FCS *fcs, const char *inFileName, const char* outFileName)
{
      int i, j, ret;
     
      char *dictEntry, len;
      unsigned int bytes;
      char bitBuf;
      int bitPos;
     
      unsigned char inBuf[FILE_BUF_SIZE];
      FILE *pfIn, *pfOut;

      pfIn = fopen(inFileName, "rb");
      if(!pfIn)
      {
            fprintf(stderr, "can't open file:%s\n", inFileName);
            return;
      }
      pfOut = fopen(outFileName, "wb");
      if(!pfOut)
      {
            fclose(pfIn);
            fprintf(stderr, "can't open file:%s\n", outFileName);
            return;
      }

      fcs_write_statistic(fcs, pfOut);

      bitBuf = 0x00;
      bitPos = 0;
      bytes = 0;
      while((ret = fread(inBuf, 1, FILE_BUF_SIZE, pfIn)) > 0)
      {
            for(i = 0; i < ret; ++i)
            {
                  len = strlen(fcs->_dictionary[inBuf[i]]);
                  dictEntry = fcs->_dictionary[inBuf[i]];
                  //printf("%s\n", dictEntry);
                  for(j = 0; j < len; ++j)
                  {
                        if(dictEntry[j] == '1')
                        {
                              bitBuf |= mask[bitPos++];
                        }
                        else
                        {
                              bitPos++;
                        }
                       
                        if(bitPos == BITS_PER_CHAR)
                        {
                              fwrite(&bitBuf, 1, sizeof(bitBuf), pfOut);
                              bitBuf = 0x00;
                              bitPos = 0;

                              bytes++;
                        }
                  }
            }
      }
      if(bitPos != 0)
      {
            fwrite(&bitBuf, 1, sizeof(bitBuf), pfOut);
            bytes++;
      }

      fclose(pfIn);
      fclose(pfOut);
      printf("The compression ratio is:%f%%\n",
            (fcs->_total - bytes) * 100.0 / fcs->_total);
}
#endif

static void fcs_read_statistic(FCS *fcs, AES *aes)
{
      int i;

      fcs->_charsCount = aes->_charsCount;

      for(i = 0; i < MAX_CHARS; ++i)
      {
            fcs->_statistic[i] = aes->_statistic[i];
            fcs->_total += fcs->_statistic[i];
      }
}

static void fcs_do_decompress(FCS *fcs, AES *aes, CBP *cbp)
{
      int i, j, bytes = 0, offset = 0;
      unsigned char ch;
      HTN *htn;
      unsigned char buf[FILE_BUF_SIZE];
      unsigned char bitCode;

      htn = fcs->_haffuman;
      bitCode = 0x00;
		for(i = 0; i < aes->buf_len; ++i)
		{
		      ch = aes->buf[i];

		      for(j = 0; j < BITS_PER_CHAR; ++j)
		      {
		            if(ch & mask[j])
		            {
		                  htn = htn->_right;     
		            }
		            else
		            {
		                  htn = htn->_left;
		            }
		            if(htn->_left == NULL && htn->_right == NULL) //leaf
		            {
		                  if(fcs->_total > 0)
		                  {
		                  		memcpy(&buf[bytes], &htn->_ch, 1);
		                  		bytes++;
								if(bytes >= FILE_BUF_SIZE){
									cbp->cb(cbp, buf, offset, FILE_BUF_SIZE);
									bytes = 0;
									offset += FILE_BUF_SIZE;
								}
		                        fcs->_total--;
		                  }
		                  htn = fcs->_haffuman;
		            }
		      }
		}
		if(bytes){
			cbp->cb(cbp, buf, offset, bytes);
		}
}


//FCS functions
FCS *fcs_new()
{
      FCS *fcs = (FCS *)malloc(sizeof(FCS));
      fcs->_charsCount = 0;
      fcs->_total = 0;
      atbm_memset(fcs->_statistic, 0, sizeof(fcs->_statistic));
      atbm_memset(fcs->_dictionary, 0, sizeof(fcs->_dictionary));
      fcs->_haffuman = NULL;
      return fcs;
}

void fcs_free(FCS *fcs)
{
      int i;
      if(fcs)
      {
            if(fcs->_haffuman)
                  htn_free(fcs->_haffuman);
            for(i = 0; i < MAX_CHARS; ++i)
                  free(fcs->_dictionary[i]);
            free(fcs);
      }
}

#if SUPPORT_COMPRESS
void fcs_compress(FCS *fcs, const char *inFileName, const char *outFileName)
{
      fprintf(stdout, "To compress file: %s ...\n", inFileName);
      fcs_generate_statistic(fcs, inFileName);
      fcs_create_haffuman_tree(fcs);
      fcs_generate_dictionary(fcs);
      fcs_do_compress(fcs, inFileName, outFileName);
      fprintf(stdout, "The compressed data of file: %s stored at %s!\n",
            inFileName, outFileName);
}
#endif
void fcs_decompress(AES *aes, CBP *cbp)
{
	  FCS *fcs;
	  fcs = fcs_new();

      fcs_read_statistic(fcs, aes);
      fcs_create_haffuman_tree(fcs);
      fcs_generate_dictionary(fcs);
      fcs_do_decompress(fcs, aes, cbp);
	  fcs_free(fcs);
}
