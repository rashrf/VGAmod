#ifndef __MODULATOR_H__
#define __MODULATOR_H__

#include <math.h>
#include "buffer.h"

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif
#define min(x,y) ((x)<(y)? (x):(y)) 
#define max(x,y) ((x)>(y)? (x):(y)) 
 
#define COS_BUFFER_SIZE_SHIFT	8  // cannot be bigger than 16 because of offset shift of 16
#define COS_BUFFER_SIZE (1<<COS_BUFFER_SIZE_SHIFT)
#define COS_BUFFER_SIZE_MASK ((1<<COS_BUFFER_SIZE_SHIFT)-1)
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
	unsigned int offset16_s;
	unsigned int offset16_c;
	int Qsign; 
} mod_parms;

typedef struct
{
	int width;
	int wtotal;
	int height;
	int htotal;
	int outputShiftRight; //shift right after multiply
	int outputMask; //clamp ouput value with mask
	int outputShiftLeft; //left shift after clamping
} VGA_Parms;

int ComplexModulate(rt_buffer * baseBandSignal, int * output, mod_parms * modParms, VGA_Parms * VGAParms)
{
	int i;
	//int offset16_cQ= *poffset16_cQ;
	int index_s, index_cI, index_cQ;
	int last_index_s=0;
	int I = ReadBufferInc(baseBandSignal,0);
	int Q = ReadBufferInc(baseBandSignal,0)*modParms->Qsign;
	int outputSize = VGAParms->width*VGAParms->height;
	int inputSize = VGAParms->wtotal*VGAParms->htotal;
	int idx=0;
	static int count=0;
	int val = rand()&0x000000FF;
	//printf("read %d %d, ",baseBandSignal->readpos,baseBandSignal->writepos);
	for(i=0; i<inputSize&&idx<outputSize; i++)
	{
		//fprintf(stderr,"(%d %d) ",i,idx);
		index_s = (i*modParms->k_s+modParms->offset16_s)>>16;
		if(index_s!=last_index_s) //check if I/Q sym needs to be updated
		{
			last_index_s=index_s;
			I = ReadBufferInc(baseBandSignal,0);
			Q = ReadBufferInc(baseBandSignal,0)*modParms->Qsign;
		}
		//	index_cI = ((i*modParms->k_c+modParms->offset16_c)>>16)%COS_BUFFER_SIZE;
		//	index_cQ = (index_cI+((3*COS_BUFFER_SIZE)>>2))%COS_BUFFER_SIZE;
			
		if(i%VGAParms->wtotal < VGAParms->width) // Output if in visible region
		{
			//output[idx] = (((I*cos_table[index_cI]+Q*cos_table[index_cQ])>>VGAParms->outputShiftRight)&VGAParms->outputMask)<<VGAParms->outputShiftLeft;
			output[idx++]=val;
		}
	}
	modParms->offset16_s=(inputSize*modParms->k_s+modParms->offset16_s)&0xFFFF;
	modParms->offset16_c=(inputSize*modParms->k_c+modParms->offset16_c)&(0xFFFF+(COS_BUFFER_SIZE_MASK<<16));
	//fprintf(stderr,"%d %d %d %d %d %d\n",VGAParms->width,VGAParms->height,VGAParms->wtotal ,VGAParms->htotal,inputSize,outputSize );
	if((i+VGAParms->wtotal-VGAParms->width)/VGAParms->wtotal != VGAParms->height)
		fprintf(stderr,"We Got a problem! %d %d\n",i/VGAParms->wtotal ,VGAParms->height);
	return idx;
}   







#endif /* ! __MODULATOR_H__ */
