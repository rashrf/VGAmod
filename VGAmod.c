#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/select.h>

#include <sys/io.h>
#include <unistd.h>
#include <sched.h>
#include <SDL/SDL.h>
#include <sys/mman.h>
//SDL_fbvideo.h
#include "modulator.h"
//#include "modulatorTest.h"
#define PCIO_OFFSET 0x00601000

#define BUFFER_SIZE_SHIFT	16
#define BUFFER_SIZE ((long long)1<<BUFFER_SIZE_SHIFT)
#define BUFFER_SAIZE_MASK (((long long)1<<BUFFER_SIZE_SHIFT)-1)

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

int readInputScale(void *thing)
{//#include <asm/io.h>
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
			{
				WriteBuffer(input_scale->buf,(short)(max(-32768.0,min(32768.0,tmpbuf[i]*input_scale->scale))));
				//printf("%d ",(short)(max(-32768.0,min(32768.0,tmpbuf[i]*input_scale->scale))));
			}
		} else 
			usleep(100000);
    }
	FlushBuffer(input_scale->buf);
    fprintf(stderr,"ReadInput Complete\n");
	return 0;
}

int main(int argc, char **argv)
{
 	VGA_Parms VGAParms;
	mod_parms modParms;
	rt_buffer baseBandBuffer; 
	input_scale_struct inputStruct;
	SDL_Surface *sf;
	SDL_Thread *thread;
	SDL_Event event;
	const SDL_VideoInfo * vi;

 	int i;
	long long sampleCount=0;
	long long sampleWrittenCount=0;
	
	int fc =  35000000;
	int rate = 146200000;
	int fs =  1500000;
	int inputShift = 0; //input>>inputShift
	int carrierShift=0; //carrier>>inputShift
	char * inputName = "stdin";
	FILE * inPipe;
	int bInvert;
	struct timeval tv1, tv2;
    unsigned int offset_s=0;
	unsigned int offset_c=0;
	int outputBufferSize;
	int * outputBuffer;
	long long k_s; //2^16*fs/rate
	long long k_c; //2^16*fc/rate
	int ts_us,us; 
	char * mapped_io;
	volatile unsigned char *port;
	
	 /* Signal parameters */
	VGAParms.width = 2048;
	VGAParms.wtotal = 0;
	VGAParms.height = 2048;
	VGAParms.htotal = 0;
	VGAParms.outputShiftRight = 0; //shift right after multiply
	VGAParms.outputMask=0xFF; //clamp ouput value with mask
	VGAParms.outputShiftLeft = 0; //left shift after clamping. Set RGBcolor ouput: 0->blue, 8->green, 16->red  
	VGAParms.outputAddOffset = 128;
	VGAParms.vsyncOffsetSignal = 0;
	VGAParms.vsyncOffsetCarrier = 0;
	
	/* Signal parameters */

    // Get Input parameters 
    while ((i = getopt(argc, argv, "w:W:h:H:r:c:s:i:o:C:I:O:M:L:a:v:V:")) != -1) 
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
				VGAParms.vsyncOffsetSignal = atoi(optarg);
				break;
			case 'V':
				VGAParms.vsyncOffsetCarrier = atoi(optarg);
				break;
			
		}
	}

	if(argc==1)
	{
		printf("Usage: VGAmod -w [width] -W [wtotal] -h [height] -H [htotal] \n");
		printf("               -r [rate] -c [CarrierFreq] -s [SampleFreq] -i [InputFile]\n");
		printf("               -C [CarrierSignalShift] -I [InputSignalShift]\n");
		printf("               -O [OutputShiftRight] -M [OutputMask] -L [OutputShiftLeft]\n");
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
	ts_us = (int)1000000.0*(VGAParms.wtotal*VGAParms.htotal)/rate;

   if(ioperm(0x3da, 0x3db, 1))
 	{
		fprintf(stderr, "ioperm fails\n") ;
		exit(-1);
	}
  		

   
 	inputStruct.buf = &baseBandBuffer;
	inputStruct.inputFile = inPipe;
	inputStruct.scale=((float)1/(1<<inputShift));
	
	thread =SDL_CreateThread(readInputScale, &inputStruct);
	if (!thread)
	{
		printf("ERROR; Could not create SDL thread\n");
        exit(-1);
    }
    fprintf(stderr, "buffering input...");
    while (IsBufferEmpty(&baseBandBuffer)) 
		usleep(100000);
    fprintf(stderr, "done %d Bytes\n",GetBufferFullness(&baseBandBuffer));

    /* Prepare SDL */
    SDL_Init(SDL_INIT_VIDEO);
    vi = SDL_GetVideoInfo();
    fprintf(stderr,"hwa: %d\n", vi->hw_available);
    fprintf(stderr,"pix: %d\n", vi->vfmt->BitsPerPixel);
	sf = SDL_SetVideoMode(VGAParms.width, VGAParms.height, 32, 
				       SDL_HWSURFACE|SDL_FULLSCREEN|
				       SDL_DOUBLEBUF);
  /*if (sf->flags & SDL_HWSURFACE)
    printf("Using hardware surface.\n");
  else
    printf("Not using hardware surface.\n");

  if (sf->flags & SDL_DOUBLEBUF)
    printf("Using double buffering.\n");
  else
    printf("Not using double buffering.\n"); */
   
 

	
  
    fprintf(stderr, "\n");
    fprintf(stderr, "SampleRate:   %3.2f MHz\n", (double)rate/1000000);
    fprintf(stderr, "Carrier:      %3.2f MHz\n", (double)fc/1000000);
    fprintf(stderr, "SymbolRate:   %3.2f\n", (double)fs);
    fprintf(stderr, "\n");

    struct sched_param sp;
    sp.sched_priority = 10;
    sched_setscheduler(0, SCHED_RR, &sp);

	gettimeofday(&tv1, 0);
	while(!IsFlushComplete(&baseBandBuffer))
	{
		if (SDL_PollEvent(&event) && event.type == SDL_MOUSEBUTTONDOWN)
			return;
		if(GetBufferFullness(&baseBandBuffer)>=outputBufferSize || IsFlushing(&baseBandBuffer))
		{
			sampleWrittenCount += ComplexModulate(&baseBandBuffer, outputBuffer, &modParms, &VGAParms);
			SDL_LockSurface(sf);
			memcpy(sf->pixels, outputBuffer, outputBufferSize* sizeof(int));
			SDL_UnlockSurface(sf);
			//sampleCount += outputBufferSize;
			gettimeofday(&tv2, 0);
			us = time_delta(&tv1, &tv2);
			us = ts_us - us;
			if (us > 0)
				usleep(us);
			gettimeofday(&tv1, 0);
			while (  (inb(0x3da) & 0x08) );
			while ( !(inb(0x3da) & 0x08) );
    		SDL_Flip(sf);
			//fprintf(stderr,"blah");
		}
	
	}
	fprintf(stderr,"Sample        : %llu\n",sampleCount);
	fprintf(stderr,"Sample Written: %llu\n",sampleWrittenCount);
	DeInitBuffer(&baseBandBuffer);
	free(outputBuffer);
	if(inPipe!=stdin)
		fclose(inPipe);
   SDL_Quit();
 }
 
 
 
 
 
