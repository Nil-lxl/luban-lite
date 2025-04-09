
#include "dsputil.h"
int ff_fft_init(FFTContext *s, int nbits, int inverse)
{
    int i, j, m, n;
    float alpha, c1, s1, s2;

    s->nbits = nbits;
    n = 1 << nbits;

    s->exptab = av_malloc((n / 2) * sizeof(MYFFTComplex32));
    if (!s->exptab)
        goto fail;
    s->revtab = av_malloc(n * sizeof(uint16_t));
    if (!s->revtab)
        goto fail;
    s->inverse = inverse;

    s2 = inverse ? 1.0 : -1.0;

    for(i=0;i<(n/2);i++)
	{
		alpha = 2 * M_PI * (float)i / (float)n;
        c1 = cos(alpha);
        s1 = sin(alpha) * s2;


        s->exptab[i].re = float2fp32(c1);
		s->exptab[i].re = s->exptab[i].re / 0x0000000000010000;
        s->exptab[i].im = float2fp32(s1);
		s->exptab[i].im = s->exptab[i].im / 0x0000000000010000;
    }
    s->fft_calc = ff_fft_calc_c;
    s->exptab1 = NULL;


    for(i=0;i<n;i++) {
        m=0;
        for(j=0;j<nbits;j++) {
            m |= ((i >> j) & 1) << (nbits-j-1);
        }
        s->revtab[i]=m;
    }
    return 0;
 fail:
    av_freep(&s->revtab);
    av_freep(&s->exptab);
    av_freep(&s->exptab1);
    return -1;
}

/* butter fly op */
#define BF(pre, pim, qre, qim, pre1, pim1, qre1, qim1) \
{\
  FFTSample ax, ay, bx, by;\
  bx=pre1;\
  by=pim1;\
  ax=qre1;\
  ay=qim1;\
  pre = (bx + ax);\
  pim = (by + ay);\
  qre = (bx - ax);\
  qim = (by - ay);\
}

#define BFCOPY(pre, pim, qre, qim, pre1, pim1, qre1, qim1) \
{\
  long long ax, ay, bx, by;\
  bx=pre1;\
  by=pim1;\
  ax=qre1;\
  ay=qim1;\
  pre = (bx + ax);\
  pim = (by + ay);\
  qre = (bx - ax);\
  qim = (by - ay);\
}

#define  fpMulExp(x, y) \
	( (x) * ( y / 0x0000000000010000))

#define  fpMulExpCopy(x, y) \
	( (x  / 0x0000000000010000 ) * ( y))

#define MUL16EXP(a, b) (fpMulExp(a, b))
#define MUL16EXPCOPY(a, b) (fpMulExpCopy(a, b))

#define CMULEXP(pre, pim, are, aim, bre, bim) \
{\
   pre = (MUL16EXP(are, bre) - MUL16EXP(aim, bim));\
   pim = (MUL16EXP(are, bim) + MUL16EXPCOPY(bre, aim));\
}

#define MUL16COPY(a, b) (fpMul(a, b))

#define CMULCOPY(pre, pim, are, aim, bre, bim) \
{\
   pre = (MUL16COPY(are, bre) - MUL16COPY(aim, bim));\
   pim = (MUL16COPY(are, bim) + MUL16COPY(bre, aim));\
}


#define MUL16(a,b) ((a) * (b))
#define CMUL(pre, pim, are, aim, bre, bim) \
{\
   pre = (MUL16(are, bre) - MUL16(aim, bim));\
   pim = (MUL16(are, bim) + MUL16(bre, aim));\
}

/**
 * Do a complex FFT with the parameters defined in ff_fft_init(). The
 * input data must be permuted before with s->revtab table. No
 * 1.0/sqrt(n) normalization is done.
 */


void ff_fft_calc_c(FFTContext *s, FFTComplex *z)
{
   int ln = s->nbits;
    int j, np, np2;
    int nblocks, nloops;


	MYFFTComplex32 *p, *q;

    MYFFTComplex32 *exptab = s->exptab;

	int l;

	long long tmp_re, tmp_im;

    np = 1 << ln;


	p = (MYFFTComplex32 *)z;
    j=(np >> 1);
    do
	{
     	BFCOPY(p[0].re, p[0].im, p[1].re, p[1].im,
	         p[0].re, p[0].im, p[1].re, p[1].im);
        p+=2;
    } while (--j != 0);

	p = (MYFFTComplex32 *)z;
    j=np >> 2;
    if (s->inverse)
	{
        do
		{
          BFCOPY(p[0].re, p[0].im, p[2].re, p[2].im,
               p[0].re, p[0].im, p[2].re, p[2].im);

		  BFCOPY(p[1].re, p[1].im, p[3].re, p[3].im,
               p[1].re, p[1].im, -p[3].im, p[3].re);

            p+=4;
        } while (--j != 0);
    }
	else
	{
        do
		{
          	/*fComplex0.re = fp2float(p[0].re);
			fComplex0.im = fp2float(p[0].im);

			fComplex1.re = fp2float(p[1].re);
			fComplex1.im = fp2float(p[1].im);

			fComplex2.re = fp2float(p[2].re);
			fComplex2.im = fp2float(p[2].im);

			fComplex3.re = fp2float(p[3].re);
			fComplex3.im = fp2float(p[3].im);
			*/

          	/*fComplex0_copy.re = (p[0].re);
			fComplex0_copy.im = (p[0].im);

			fComplex1_copy.re = (p[1].re);
			fComplex1_copy.im = (p[1].im);

			fComplex2_copy.re = (p[2].re);
			fComplex2_copy.im = (p[2].im);

			fComplex3_copy.re = (p[3].re);
			fComplex3_copy.im = (p[3].im);
			*/
/*
            BF(fComplex0.re, fComplex0.im, fComplex2.re, fComplex2.im,
               fComplex0.re, fComplex0.im, fComplex2.re, fComplex2.im);

            BF(fComplex1.re, fComplex1.im, fComplex3.re, fComplex3.im,
               fComplex1.re, fComplex1.im, fComplex3.im, -fComplex3.re);
*/
           /*
			BFCOPY(fComplex0_copy.re, fComplex0_copy.im, fComplex2_copy.re, fComplex2_copy.im,
               fComplex0_copy.re, fComplex0_copy.im, fComplex2_copy.re, fComplex2_copy.im);

            BFCOPY(fComplex1_copy.re, fComplex1_copy.im, fComplex3_copy.re, fComplex3_copy.im,
               fComplex1_copy.re, fComplex1_copy.im, fComplex3_copy.im, -fComplex3_copy.re);
			*/

			/*p[0].re = float2fp(fComplex0.re);
			p[0].im = float2fp(fComplex0.im);

			p[1].re = float2fp(fComplex1.re);
			p[1].im = float2fp(fComplex1.im);

			p[2].re = float2fp(fComplex2.re);
			p[2].im = float2fp(fComplex2.im);

			p[3].re = float2fp(fComplex3.re);
			p[3].im = float2fp(fComplex3.im);
			*/
			/*
			p[0].re = (fComplex0_copy.re);
			p[0].im = (fComplex0_copy.im);

			p[1].re = (fComplex1_copy.re);
			p[1].im = (fComplex1_copy.im);

			p[2].re = (fComplex2_copy.re);
			p[2].im = (fComplex2_copy.im);

			p[3].re = (fComplex3_copy.re);
			p[3].im = (fComplex3_copy.im);
			*/

			 BF(p[0].re, p[0].im, p[2].re, p[2].im,
               p[0].re, p[0].im, p[2].re, p[2].im);
            BF(p[1].re, p[1].im, p[3].re, p[3].im,
               p[1].re, p[1].im, p[3].im, -p[3].re);
            p+=4;
        } while (--j != 0);
    }


    nblocks = np >> 3;
    nloops = 1 << 2;
    np2 = np >> 1;
     do {
		p = (MYFFTComplex32 *)z;
        q = p + nloops;
        for (j = 0; j < nblocks; ++j)
		{

			/*fComplexQ.re = fp2float(q->re);
			fComplexQ.im = fp2float(q->im);

			fComplexP.re = fp2float(p->re);
			fComplexP.im = fp2float(p->im);


			fComplexQ_copy.re = (q->re);
			fComplexQ_copy.im = (q->im);

			fComplexP_copy.re = (p->re);
			fComplexP_copy.im = (p->im);*/

/*
            BF(fComplexP.re, fComplexP.im, fComplexQ.re, fComplexQ.im,
               fComplexP.re, fComplexP.im, fComplexQ.re, fComplexQ.im);
			*/
			/*
			BFCOPY(fComplexP_copy.re, fComplexP_copy.im, fComplexQ_copy.re, fComplexQ_copy.im,
               fComplexP_copy.re, fComplexP_copy.im, fComplexQ_copy.re, fComplexQ_copy.im);*/

			/*
			p->re = float2fp(fComplexP.re);
			p->im = float2fp(fComplexP.im);

			q->re = float2fp(fComplexQ.re);
			q->im = float2fp(fComplexQ.im);
*/
			/*
			p->re = (fComplexP_copy.re);
			p->im = (fComplexP_copy.im);

			q->re = (fComplexQ_copy.re);
			q->im = (fComplexQ_copy.im);
			*/

			BFCOPY(p->re, p->im, q->re, q->im,
               p->re, p->im, q->re, q->im);

            p++;
            q++;

            for(l = nblocks; l < np2; l += nblocks)
			{
				/*
				fComplexQ.re = fp2float(q->re);
				fComplexQ.im = fp2float(q->im);

				fComplexP.re = fp2float(p->re);
				fComplexP.im = fp2float(p->im);
				*/

				/*fComplexQ_copy.re = (q->re);
				fComplexQ_copy.im = (q->im);

				fComplexP_copy.re = (p->re);
				fComplexP_copy.im = (p->im);
				*/
				/*
				expValue.re = fp2float(exptab[l].re);
				expValue.im = fp2float(exptab[l].im);
				*/

				/*expValue_copy.re = (exptab[l].re);
				expValue_copy.im = (exptab[l].im);*/
/*
				CMUL(tmp_re, tmp_im, expValue.re, expValue.im, fComplexQ.re, fComplexQ.im);

				BF(fComplexP.re, fComplexP.im, fComplexQ.re, fComplexQ.im,
					   fComplexP.re, fComplexP.im, tmp_re, tmp_im);*/

/*
				CMULCOPY(tmp_re, tmp_im, expValue_copy.re, expValue_copy.im, fComplexQ_copy.re, fComplexQ_copy.im);

				BFCOPY(fComplexP_copy.re, fComplexP_copy.im, fComplexQ_copy.re, fComplexQ_copy.im,
					   fComplexP_copy.re, fComplexP_copy.im, tmp_re, tmp_im);*/
/*
				p->re = float2fp(fComplexP.re);
				p->im = float2fp(fComplexP.im);

				q->re = float2fp(fComplexQ.re);
				q->im = float2fp(fComplexQ.im);
*/

				/*p->re = (fComplexP_copy.re);
				p->im = (fComplexP_copy.im);

				q->re = (fComplexQ_copy.re);
				q->im = (fComplexQ_copy.im);
*/

				CMULEXP(tmp_re, tmp_im, exptab[l].re, exptab[l].im, q->re, q->im);

                BFCOPY(p->re, p->im, q->re, q->im,
                   p->re, p->im, tmp_re, tmp_im);

                p++;
                q++;
            }

            p += nloops;
            q += nloops;
        }
        nblocks = nblocks >> 1;
        nloops = nloops << 1;
    } while (nblocks != 0);


}


/**
 * Do the permutation needed BEFORE calling ff_fft_calc()
 */
void ff_fft_permute(FFTContext *s, FFTComplex *z)
{
    int j, k, np;
    FFTComplex tmp;
    const uint16_t *revtab = s->revtab;

    /* reverse */
    np = 1 << s->nbits;
    for(j=0;j<np;j++) {
        k = revtab[j];
        if (k < j) {
            tmp = z[k];
            z[k] = z[j];
            z[j] = tmp;
        }
    }
}

void ff_fft_end(FFTContext *s)
{
    av_freep(&s->revtab);
    av_freep(&s->exptab);
    av_freep(&s->exptab1);
}

