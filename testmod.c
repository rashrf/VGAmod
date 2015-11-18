#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
//#include <sys/select.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>

#include "modulator.h"

#define BUFFER_SIZE_SHIFT	16
#define BUFFER_SIZE (1<<BUFFER_SIZE_SHIFT)
#define BUFFER_SAIZE_MASK ((1<<BUFFER_SIZE_SHIFT)-1)




 /* Calculate the difference in 'us' between two timevals */
inline long int time_delta(struct timeval *tv1, struct timeval *tv2)
{
    long int us;
    us = (tv2->tv_sec - tv1->tv_sec)*1000000 + tv2->tv_usec;
    us = us - tv1->tv_usec;
    return us;
}






typedef struct
{
	rt_buffer * buf;
	float scale;
	FILE * inputFile;
} input_scale_struct;

void * readInputScale(void *thing)
{
    int len,i;
	float tmpbuf[1024];
	int readComplete=0;
	input_scale_struct * input_scale = (input_scale_struct *)thing;
    while (!readComplete) 
	{
		len = GetWriteLen(input_scale->buf,1024);
		if (len > 0) 
		{
			len = fread(tmpbuf, sizeof(float), len, input_scale->inputFile);
			readComplete = (len == 0);
			for (i=0; i < len; i++) 
				WriteBuffer(input_scale->buf,(short)(max(-32768.0,min(32768.0,tmpbuf[i]*input_scale->scale))));
		} else 
			usleep(100000);
    }
	FlushBuffer(input_scale->buf);
    fprintf(stderr,"ReadInput Complete\n");
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	VGA_Parms VGAParms;
	mod_parms modParms;
	rt_buffer baseBandBuffer; 
	input_scale_struct inputStruct;
	pthread_t thread;
    int i;
	long long sampleCount=0;
	long long sampleWrittenCount=0;
	
	int fc =  35000000;
	int rate = 146200000;
	int fs =  1500000;
	int inputShift = 0; //input>>inputShift
	int carrierShift=0; //carrier>>inputShift
	char * inputName = "stdin";
	char * outputName = "stdout";
	FILE * inPipe;
	FILE * outPipe;
	int bInvert;
	struct timeval tv1, tv2;
    unsigned int offset_s=0;
	unsigned int offset_c=0;
	int outputBufferSize;
	int * outputBuffer;
	long long k_s; //2^16*fs/rate
	long long k_c; //2^16*fc/rate
	int ts_us,us; 
	
	VGAParms.width = 40;
	VGAParms.wtotal = 50;
	VGAParms.height = 40;
	VGAParms.htotal = 42;
	VGAParms.outputShiftRight = 0; //shift right after multiply
	VGAParms.outputMask=0xFFFFFFFF; //clamp ouput value with mask
	VGAParms.outputShiftLeft = 0; //left shift after clamping
	VGAParms.outputAddOffset = 0;
	VGAParms.vsyncOffset = 0;

	/* Signal parameters */

    // Get Input parameters 
    while ((i = getopt(argc, argv, "w:W:h:H:r:c:s:i:o:C:I:O:a:v:")) != -1) 
    {
		switch(i) 
		{
			case 'w':
				VGAParms.width = atoi(optarg);
				break;
			case 'W':
				VGAParms.wtotal = atoi(optarg);
				break;
			case 'h':
				VGAParms.height = atoi(optarg);
				break;
			case 'H':
				VGAParms.htotal = atoi(optarg);
				break;
			case 'r':
				rate = atoi(optarg);
				break;
			case 'c':
				fc = atoi(optarg);
				break;
			case 's':
				fs = atoi(optarg);
				break;
			case 'i':
				inputName = strdup(optarg);
				break;
			case 'o':
				outputName = strdup(optarg);
				break;
			case 'C':
				carrierShift = atoi(optarg);
				break;
			case 'I':
				inputShift = atoi(optarg);
				break;
			case 'O':
				VGAParms.outputShiftRight = atoi(optarg);
				break;
			case 'M':
				sscanf(optarg, "%x", &VGAParms.outputMask);
				break;			
			case 'L':
				VGAParms.outputShiftLeft = atoi(optarg);
				break;
			case 'a':
				VGAParms.outputAddOffset = atoi(optarg);
				break;
			case 'v':
				VGAParms.vsyncOffset = atoi(optarg);
				break;

		}
	}
	if(argc==1)
	{
		printf("Usage: testmod -w [width] -W [wtotal] -h [height] -H [htotal] \n");
		printf("               -r [rate] -c [CarrierFreq] -s [SampleFreq] -i [InputFile]\n");
		printf("               -o [outfile]  -C [CarrierSignalShift] -I [InputSignalShift]\n");
		printf("               -O [OutputShiftRight] -M [OutputMask] -L [OutputShiftLeft]\n");
		printf("               -f [OutputAddOffset] \n");
 	}

	if(strcmp(inputName,"stdin")!=0)
	{
		inPipe = fopen(inputName,"rb");
		if(!inPipe) 
		{
			fprintf(stderr, "Cannon open file %s",inputName);
			exit(-1);
		}
	}else
		inPipe= stdin;
		
	if(strcmp(outputName,"stdout")!=0)
	{
		outPipe = fopen(outputName,"wb");
		if(!outPipe) 
		{
			fprintf(stderr, "Cannon open file %s",outputName);
			exit(-1);
		}
	}else
		outPipe= stdout;
	VGAParms.wtotal = max(VGAParms.wtotal,VGAParms.width);
	VGAParms.htotal = max(VGAParms.htotal,VGAParms.height);
	
	if(fc<0)
	{
		fc=-fc;
		modParms.Qsign=-1;
	}else
		modParms.Qsign=1;
	modParms.k_s = ((long long)(1<<OFFSET_BITS))*fs/rate;
	modParms.k_c = ((long long) (1<<OFFSET_BITS))*fc*COS_BUFFER_SIZE/rate;
	modParms.offset_s=0;
	modParms.offset_c=0;

    outputBufferSize = VGAParms.width*VGAParms.height;
	outputBuffer = malloc(	outputBufferSize*sizeof(int));
	InitBuffer(&baseBandBuffer,outputBufferSize);
   	initCosineTable((float)1/(1<<carrierShift));
	ts_us = 1000000/fs;

	inputStruct.buf = &baseBandBuffer;
	inputStruct.inputFile = inPipe;
	inputStruct.scale=((float)1/(1<<inputShift));

	if (pthread_create(&thread, NULL, readInputScale, &inputStruct))
	{
		printf("ERROR; Could not create read thread\n");
        exit(-1);
    }
  

    fprintf(stderr, "buffering input...");
    while (IsBufferEmpty(&baseBandBuffer)) 
		usleep(100000);
    fprintf(stderr, "done %d\n",GetBufferFullness(&baseBandBuffer));

    fprintf(stderr, "\n");
    fprintf(stderr, "SampleRate:   %3.2f MHz\n", (double)rate/1000000);
    fprintf(stderr, "Carrier:      %3.2f MHz\n", (double)fc/1000000);
    fprintf(stderr, "SymbolRate:   %3.2f\n", (double)fs);
    fprintf(stderr, "\n");
	

	gettimeofday(&tv1, 0);
	while(!IsFlushComplete(&baseBandBuffer))
	{
		if(GetBufferFullness(&baseBandBuffer)>=outputBufferSize || IsFlushing(&baseBandBuffer))
		{
			sampleWrittenCount += ComplexModulate(&baseBandBuffer, outputBuffer, &modParms, &VGAParms);
			sampleCount += fwrite(outputBuffer, sizeof(int), outputBufferSize, outPipe);
			gettimeofday(&tv2, 0);
			us = time_delta(&tv1, &tv2);
			us = ts_us - us;
			if (us > 0)
				usleep(us);
			gettimeofday(&tv1, 0);
		}
	
	}
	fprintf(stderr,"Sample        : %llu\n",sampleCount);
	fprintf(stderr,"Sample Written: %llu\n",sampleWrittenCount);

	DeInitBuffer(&baseBandBuffer);
	if(inPipe!=stdin)
		fclose(inPipe);
	if(outPipe!=stdout)
		fclose(outPipe);
	pthread_exit(NULL);
 }
