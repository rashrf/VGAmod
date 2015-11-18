#ifndef __BUFFER_H__
#define __BUFFER_H__
#define ASSERT(x)
//real time buffer of shorts
typedef enum {EMPTY=0,WORKING,FULL} buffer_state;
typedef struct 
{
	short		* buffer;
	int			size; 
	int		 	bFlush;
	volatile int writepos;
	volatile int readpos;
} rt_buffer;

inline int IsBufferEmpty(rt_buffer * buf) {return buf->writepos==buf->readpos;}
inline void FlushBuffer(rt_buffer * buf) {buf->bFlush=1;}
inline int IsFlushing(rt_buffer * buf) {return !IsBufferEmpty(buf)&&buf->bFlush==1;}
inline int IsFlushComplete(rt_buffer * buf) {return IsBufferEmpty(buf)&&buf->bFlush==1;}
int InitBuffer(rt_buffer * buf,int size)
{
	buf->size = size+1;// when full, last wrtiepos still empty(distinguish between full/emppty state)
	buf->buffer = malloc((size+1) * sizeof(short)); 
	buf->writepos=0;
	buf->readpos = 0;
	buf->bFlush=0;
}

void DeInitBuffer(rt_buffer * buf)
{
	if(buf && buf->buffer)
		free(buf->buffer);
}

inline int WriteBuffer(rt_buffer * buf,short value)
{
	if((buf->writepos+1)%buf->size!=buf->readpos)
	{
		buf->buffer[buf->writepos++]=value; 
		buf->writepos %= buf->size;
		return 1;
	}else //empty
		return 0;
}

inline short ReadBuffer(rt_buffer * buf,int * pSuccess)
{
	if(buf->writepos!=buf->readpos)
	{
		if(pSuccess) *pSuccess=1;
		return buf->buffer[buf->readpos];	
	}
	if(pSuccess) *pSuccess=0;
	return 0;
}

inline short ReadBufferNext(rt_buffer * buf,int * pSuccess)
{
	if(GetBufferFullness(buf)>1)
	{
		if(pSuccess) *pSuccess=1;
		return buf->buffer[(buf->readpos+1)%buf->size];	
	}
	if(pSuccess) *pSuccess=0;
	return 0;
}

inline short ReadBufferInc(rt_buffer * buf,int * pSuccess)
{
	short return_val=0;
	if(buf->writepos!=buf->readpos)
	{
		return_val = buf->buffer[buf->readpos++];
		buf->readpos %= buf->size;
		if(pSuccess) *pSuccess=1;
		return return_val;	
	}
	if(pSuccess) *pSuccess=0;
	return return_val;
}
int IncReadBufferPtr(rt_buffer * buf)
{
	if(buf->writepos!=buf->readpos)
	{
		buf->readpos = (buf->readpos+1)%buf->size;
		return  1;	
	}
	return 0;
}

inline int GetWriteLen(rt_buffer * buf, int maxlen)
{
    int len = buf->size-1-GetBufferFullness(buf);
    return len > maxlen ? maxlen : len;
}

inline int GetBufferFullness(rt_buffer * buf)
{
    int len, wp = buf->writepos;
    if (wp < buf->readpos)
		wp += buf->size;
    return  wp - buf->readpos;
}



#endif /* ! __BUFFER_H__ */
