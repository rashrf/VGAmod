#ifndef __MODULATOR_H__
#define __MODULATOR_H__

#include <math.h>
#include "buffer.h"

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif
#define min(x,y) ((x)<(y)? (x):(y)) 
#define max(x,y) ((x)>(y)? (x):(y)) 

#define OFFSET_BITS				26  // should be able to go to 56, but for some reason 24 is the max (finer freq. resolution)
#define OFFSET_BITS_MASK		(((long long)1<<OFFSET_BITS)-1) 
#define COS_BUFFER_SIZE_SHIFT	8  // cannot be bigger than 32-OFFSET_BITS because of offset shift of 16
#define COS_BUFFER_SIZE ((long long)1<<COS_BUFFER_SIZE_SHIFT)
#define COS_BUFFER_SIZE_MASK (((long long)1<<COS_BUFFER_SIZE_SHIFT)-1)
short  cos_table[COS_BUFFER_SIZE];

// Setup cos_table. scale is a number [0 1], where 1 uses full range [-32768 32767]
void initCosineTable(float scale)
{
	int i;
	for(i=0;i<COS_BUFFER_SIZE;i++)
		cos_table[i]=(short)min(32767.0,max(-32768.0,scale*32768.0*cos(2*M_PI*i/COS_BUFFER_SIZE)))	;
}

typedef struct
{
	long long k_s;
	long long k_c;
	long long offset_s;
	long long offset_c;
	int Qsign; 
} mod_parms;

typedef struct
{
	int width;
	int wtotal;
	int height;
	int htotal;
	int outputShiftRight; //shift right after multiply
	int outputAddOffset;  // add offset after shift 
	int outputMask; //clamp ouput value with mask
	int outputShiftLeft; //left shift after clamping
	int vsyncOffsetCarrier; //exact # of samples in frame  off from wtotal*htotal)
	int vsyncOffsetSignal; //exact # of samples in frame  off from wtotal*htotal)
} VGA_Parms;

int ComplexModulate(rt_buffer * baseBandSignal, int * output, mod_parms * modParms, VGA_Parms * VGAParms)
{
	int i;
	int index_s, index_cI, index_cQ;
	int last_index_s=-1;
	int bFirst=1;
	int I,Q; 
	int outputSize = VGAParms->width*VGAParms->height;
	int inputSize = VGAParms->wtotal*VGAParms->htotal;
	int idx=0;
	static int count=0;

	for(i=0; i<inputSize&&idx<outputSize; i++)
	{
		index_s = ((long long)i*modParms->k_s+modParms->offset_s)>>OFFSET_BITS;
		if(index_s!=last_index_s) //check if I/Q sym needs to be updated
		{
			last_index_s=index_s;
			if(bFirst)
				bFirst=0;
			else
			{
				IncReadBufferPtr(baseBandSignal);IncReadBufferPtr(baseBandSignal);
			}
			I = ReadBuffer(baseBandSignal,0);
			Q = ReadBufferNext(baseBandSignal,0)*modParms->Qsign;

		}
		if(i%VGAParms->wtotal < VGAParms->width) // Output if in visible region
		{
			index_cI = (((long long)i*modParms->k_c+modParms->offset_c)>>OFFSET_BITS)%COS_BUFFER_SIZE;
			index_cQ = (index_cI+((3*COS_BUFFER_SIZE)>>2))%COS_BUFFER_SIZE;
			output[idx++] = ((((I*cos_table[index_cI]+Q*cos_table[index_cQ])>>VGAParms->outputShiftRight)+VGAParms->outputAddOffset)&VGAParms->outputMask)<<VGAParms->outputShiftLeft;
		}
		
	}
	modParms->offset_s=(((long long)inputSize+VGAParms->vsyncOffsetSignal)*modParms->k_s+modParms->offset_s)&OFFSET_BITS_MASK;
		
	modParms->offset_c=(((long long)inputSize+VGAParms->vsyncOffsetCarrier)*modParms->k_c+modParms->offset_c)&(OFFSET_BITS_MASK+(COS_BUFFER_SIZE_MASK<<OFFSET_BITS));
	if((i+VGAParms->wtotal-VGAParms->width) != VGAParms->height*VGAParms->wtotal)
		fprintf(stderr,"We Got a problem! %d %d\n",i/VGAParms->wtotal ,VGAParms->height);

	return idx;
}   







#endif /* ! __MODULATOR_H__ */
