#include <stdlib.h>
#include <string.h>

unsigned int Initial_Decoder();
int Decoding_Image(unsigned char* img_buffer, int width, int height);
unsigned int GetResultLength();
int GetDecoderResult(unsigned char * result);
void Set_Donfig_Decoder(int type, int tag);