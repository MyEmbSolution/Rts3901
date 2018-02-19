
#include <stdio.h>
#include <stdfix.h>
#include <math.h>

typedef float FAST_FLOAT;
const int DCTSIZE = 8;
/*
 * Perform the forward DCT on one block of samples.
 */

void
jpeg_fdct_float (FAST_FLOAT * indata, FAST_FLOAT *outdata, FAST_FLOAT *temp)
{
  FAST_FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  FAST_FLOAT tmp10, tmp11, tmp12, tmp13;
  FAST_FLOAT z1, z2, z3, z4, z5, z11, z13;
  FAST_FLOAT *indataptr;
  FAST_FLOAT *pbuf;
  int ctr;
  int out_ctr;

  /* Pass 1: process rows. */

  indataptr = indata;
  pbuf = temp;

  for (out_ctr = 1; out_ctr >=0; out_ctr--) {
    for (ctr = 8-1; ctr >= 0; ctr--) {
      tmp0 = indataptr[0] + indataptr[7];
      tmp7 = indataptr[0] - indataptr[7];
      tmp1 = indataptr[1] + indataptr[6];
      tmp6 = indataptr[1] - indataptr[6];
      tmp2 = indataptr[2] + indataptr[5];
      tmp5 = indataptr[2] - indataptr[5];
      tmp3 = indataptr[3] + indataptr[4];
      tmp4 = indataptr[3] - indataptr[4];
      
      /* Even part */
      
      tmp10 = tmp0 + tmp3;	/* phase 2 */
      tmp13 = tmp0 - tmp3;
      tmp11 = tmp1 + tmp2;
      tmp12 = tmp1 - tmp2;
      
      pbuf[0*8] = tmp10 + tmp11; /* phase 3 */
      pbuf[4*8] = tmp10 - tmp11;
      
      z1 = (tmp12 + tmp13) * ((FAST_FLOAT) 0.541196100); /* c4 */
    
      pbuf[2*8] = (z1 + tmp13 *  0.765366865);
      pbuf[6*8] = (z1 + tmp12 * -1.847759065);
      
      /* Odd part */

      z1 = tmp4 + tmp7;
      z2 = tmp5 + tmp6;
      z3 = tmp4 + tmp6;
      z4 = tmp5 + tmp7;
      z5 = (z3 + z4) * 1.175875602; /* sqrt(2) * c3 */
      
      tmp4 = tmp4 * 0.298631336; /* sqrt(2) * (-c1+c3+c5-c7) */
      tmp5 = tmp5 * 2.053119869; /* sqrt(2) * ( c1+c3-c5+c7) */
      tmp6 = tmp6 * 3.072711026; /* sqrt(2) * ( c1+c3+c5-c7) */
      tmp7 = tmp7 * 1.501321110; /* sqrt(2) * ( c1+c3-c5-c7) */
      z1 = z1 * -0.899976223; /* sqrt(2) * (c7-c3) */
      z2 = z2 * -2.562915447; /* sqrt(2) * (-c1-c3) */
      z3 = z3 * -1.961570560; /* sqrt(2) * (-c3-c5) */
      z4 = z4 * -0.390180644; /* sqrt(2) * (c5-c3) */
      
      z3 += z5;
      z4 += z5;
      
      pbuf[7*8] = tmp4 + z1 + z3;
      pbuf[5*8] = tmp5 + z2 + z4;
      pbuf[3*8] = tmp6 + z2 + z3;
      pbuf[1*8] = tmp7 + z1 + z4;

      indataptr += 8;		/* advance pointer to next row */
      pbuf += 1;		/* advance pointer to next row */
    }
    indataptr = temp;
    pbuf = outdata;
  }
}

void
jpeg_fdct_fix (_Accum * indata, _Accum *outdata, _Accum *temp)
{
  _Accum tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  _Accum tmp10, tmp11, tmp12, tmp13;
  _Accum z1, z2, z3, z4, z5, z11, z13;
  _Accum *indataptr;
  _Accum *pbuf;
  int ctr;
  int out_ctr;
  
  /* Pass 1: process rows. */
  
  indataptr = indata;
  pbuf = temp;
  
  for (out_ctr = 1; out_ctr >=0; out_ctr--) {
    for (ctr = 8-1; ctr >= 0; ctr--) {
      tmp0 = indataptr[0] + indataptr[7];
      tmp7 = indataptr[0] - indataptr[7];
      tmp1 = indataptr[1] + indataptr[6];
      tmp6 = indataptr[1] - indataptr[6];
      tmp2 = indataptr[2] + indataptr[5];
      tmp5 = indataptr[2] - indataptr[5];
      tmp3 = indataptr[3] + indataptr[4];
      tmp4 = indataptr[3] - indataptr[4];
      
      /* Even part */
      
      tmp10 = tmp0 + tmp3;	/* phase 2 */
      tmp13 = tmp0 - tmp3;
      tmp11 = tmp1 + tmp2;
      tmp12 = tmp1 - tmp2;
      
      pbuf[0*8] = tmp10 + tmp11; /* phase 3 */
      pbuf[4*8] = tmp10 - tmp11;
      
      z1 = (tmp12 + tmp13) * (0.541196100k); /* c4 */
      
      pbuf[2*8] = (z1 + tmp13 *  0.765366865k);
      pbuf[6*8] = (z1 + tmp12 * -1.847759065k);
      
      /* Odd part */
      
      z1 = tmp4 + tmp7;
      z2 = tmp5 + tmp6;
      z3 = tmp4 + tmp6;
      z4 = tmp5 + tmp7;
      z5 = (z3 + z4) * 1.175875602k; /* sqrt(2) * c3 */
      
      tmp4 = tmp4 * 0.298631336k; /* sqrt(2) * (-c1+c3+c5-c7) */
      tmp5 = tmp5 * 2.053119869k; /* sqrt(2) * ( c1+c3-c5+c7) */
      tmp6 = tmp6 * 3.072711026k; /* sqrt(2) * ( c1+c3+c5-c7) */
      tmp7 = tmp7 * 1.501321110k; /* sqrt(2) * ( c1+c3-c5-c7) */
      z1 = z1 * -0.899976223k; /* sqrt(2) * (c7-c3) */
      z2 = z2 * -2.562915447k; /* sqrt(2) * (-c1-c3) */
      z3 = z3 * -1.961570560k; /* sqrt(2) * (-c3-c5) */
      z4 = z4 * -0.390180644k; /* sqrt(2) * (c5-c3) */
      
      z3 += z5;
      z4 += z5;
      
      pbuf[7*8] = tmp4 + z1 + z3;
      pbuf[5*8] = tmp5 + z2 + z4;
      pbuf[3*8] = tmp6 + z2 + z3;
      pbuf[1*8] = tmp7 + z1 + z4;
      
      indataptr += 8;		/* advance pointer to next row */
      pbuf += 1;		/* advance pointer to next row */
    }
    indataptr = temp;
    pbuf = outdata;
  }
}


typedef short int16_t;
typedef int int32_t;

void fdct_8x8 (int16_t * indata, int16_t * outdata, int16_t * temp)
{
  int32_t tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  int32_t tmp10, tmp11, tmp12, tmp13;
  int32_t z1, z2, z3, z4, z5;

  int16_t *indataptr;

  //int16_t buf[64];
  int16_t *pbuf;

  int32_t ctr;
  int32_t out_ctr;

  int32_t shift;


  // first pass
  indataptr = indata;
  pbuf = temp;
  shift = 10;

  for(out_ctr = 1; out_ctr >= 0; out_ctr--) {
    for (ctr = (8)-1; ctr >= 0; ctr--) {
      tmp0 = indataptr[0] + indataptr[7];
      tmp7 = indataptr[0] - indataptr[7];
      tmp1 = indataptr[1] + indataptr[6];
      tmp6 = indataptr[1] - indataptr[6];
      tmp2 = indataptr[2] + indataptr[5];
      tmp5 = indataptr[2] - indataptr[5];
      tmp3 = indataptr[3] + indataptr[4];
      tmp4 = indataptr[3] - indataptr[4];

      tmp10 = tmp0 + tmp3;
      tmp13 = tmp0 - tmp3;
      tmp11 = tmp1 + tmp2;
      tmp12 = tmp1 - tmp2;

      //pbuf[0*8] = (short) ((tmp10 + tmp11) << 2);
      //pbuf[4*8] = (short) ((tmp10 - tmp11) << 2);

      pbuf[0*8] = (short) ((((tmp10 + tmp11) << 13)+ (((int) 1) << shift))>>(shift+1));
      pbuf[4*8] = (short) ((((tmp10 - tmp11) << 13)+ (((int) 1) << shift))>>(shift+1));

      z1 = (((short) (tmp12 + tmp13)) * ((short) (((int) 4433))));
      pbuf[2*8] = (short) (((z1 + (((short) (tmp13)) * ((short) (((int) 6270))))) + (((int) 1) << shift)) >> (shift+1));
      pbuf[6*8] = (short) (((z1 + (((short) (tmp12)) * ((short) (- ((int) 15137))))) + (((int) 1) << shift)) >> (shift+1));


      z1 = tmp4 + tmp7;
      z2 = tmp5 + tmp6;
      z3 = tmp4 + tmp6;
      z4 = tmp5 + tmp7;
      z5 = (((short) (z3 + z4)) * ((short) (((int) 9633))));

      tmp4 = (((short) (tmp4)) * ((short) (((int) 2446))));
      tmp5 = (((short) (tmp5)) * ((short) (((int) 16819))));
      tmp6 = (((short) (tmp6)) * ((short) (((int) 25172))));
      tmp7 = (((short) (tmp7)) * ((short) (((int) 12299))));
      z1 = (((short) (z1)) * ((short) (- ((int) 7373))));
      z2 = (((short) (z2)) * ((short) (- ((int) 20995))));
      z3 = (((short) (z3)) * ((short) (- ((int) 16069))));
      z4 = (((short) (z4)) * ((short) (- ((int) 3196))));

      z3 += z5;
      z4 += z5;

      pbuf[7*8] = (short) (((tmp4 + z1 + z3) + (((int) 1) << shift)) >> (shift+1));
      pbuf[5*8] = (short) (((tmp5 + z2 + z4) + (((int) 1) << shift)) >> (shift+1));
      pbuf[3*8] = (short) (((tmp6 + z2 + z3) + (((int) 1) << shift)) >> (shift+1));
      pbuf[1*8] = (short) (((tmp7 + z1 + z4) + (((int) 1) << shift)) >> (shift+1));

      indataptr += (8);
      pbuf += (1);
    }

  // second pass
  indataptr = temp;
  pbuf = outdata;
  shift = 17;
  }
  return;
}

double coslu[8][8];

/* Routine to initialise the cosine lookup table */
void dct_init(void)
{
  int a,b;
  double tmp;

  for(a=0;a<8;a++)
    for(b=0;b<8;b++) {
      tmp = cos((double)((a+a+1)*b) * (3.14159265358979323846 / 16.0));
      if(b==0)
	tmp /= sqrt(2.0);
      coslu[a][b] = tmp * 0.5;
    }
}

void ref_fdct (short* block)
{
  int x,y,u,v;
  double tmp, tmp2;
  double res[8][8];

  for (v=0; v<8; v++) {
    for (u=0; u<8; u++) {
      tmp = 0.0;
      for (y=0; y<8; y++) {
	tmp2 = 0.0;
	for (x=0; x<8; x++) {
	  tmp2 += (double) block[y*8+x] * coslu[x][u];
	}
	tmp += coslu[y][v] * tmp2;
      }
      res[v][u] = tmp;
    }
  }

  for (v=0; v<8; v++) {
    for (u=0; u<8; u++) {
      tmp = res[v][u];
      if (tmp < 0.0) {
	x = - ((int) (0.5 - tmp));
      } else {
	x = (int) (tmp + 0.5);
      }
      block[v*8+u] = (short) x;
    }
  }
}
short blocki[64] =
{
     8,  -166,   -97,    18,   230,  -168,   104,  -140,
    -2,  -192,  -213,   -56,  -114,   -67,   248,    19,
   137,    75,   137,   144,   166,  -178,    65,   -94,
   -78,   214,    11,   -50,    55,   147,   221,   190,
   188,    90,   133,    42,   -56,   -73,  -153,   168,
   -43,   -18,   246,  -191,  -147,   235,   122,   -46,
   144,   133,   234,  -241,   -92,   132,  -131,    46,
  -233,   234,   -92,  -225,   -29,   213,    37,  -195
};
float blockf[64] =
{
     8,  -166,   -97,    18,   230,  -168,   104,  -140,
    -2,  -192,  -213,   -56,  -114,   -67,   248,    19,
   137,    75,   137,   144,   166,  -178,    65,   -94,
   -78,   214,    11,   -50,    55,   147,   221,   190,
   188,    90,   133,    42,   -56,   -73,  -153,   168,
   -43,   -18,   246,  -191,  -147,   235,   122,   -46,
   144,   133,   234,  -241,   -92,   132,  -131,    46,
  -233,   234,   -92,  -225,   -29,   213,    37,  -195
};
_Accum blockfix[64] =
{
8,  -166,   -97,    18,   230,  -168,   104,  -140,
-2,  -192,  -213,   -56,  -114,   -67,   248,    19,
137,    75,   137,   144,   166,  -178,    65,   -94,
-78,   214,    11,   -50,    55,   147,   221,   190,
188,    90,   133,    42,   -56,   -73,  -153,   168,
-43,   -18,   246,  -191,  -147,   235,   122,   -46,
144,   133,   234,  -241,   -92,   132,  -131,    46,
-233,   234,   -92,  -225,   -29,   213,    37,  -195
};

int main(void)
{
	float outf[64];
	float tempf[64];
	short outi[64];
	short tempi[64];
  _Accum outfix[64];
  _Accum tempfix[64];

	jpeg_fdct_float(blockf, outf, tempf);
	//jpeg_fdct_float(blockf);

	fdct_8x8(blocki, outi, tempi);

  jpeg_fdct_fix(blockfix, outfix, tempfix);
	
  return 0;
}
