#include "rs485.h"
#include "bsp.h"

typedef struct
{
#define		WIDTH		8 // no larger than 8
#define		SIZE		(1<<WIDTH)
#define		MASK		(SIZE-1)
#define		NEXT(x)		((x+1)&MASK)
	uint8_t buffer[SIZE];
	uint8_t wptr;
	uint8_t rptr;
}
TxFifo_t;

static TxFifo_t fifo;

static void FifoInit(void)
{
	fifo.wptr = 0;
	fifo.rptr = 0;
}

static int FifoWrite(uint8_t wd)
{
	if( NEXT(fifo.wptr)==fifo.rptr ) return -1; // full
	fifo.buffer[fifo.wptr] = wd;
	fifo.wptr = NEXT(fifo.wptr);
	return wd;
}

static int FifoRead(void)
{
	uint8_t rd;
	if( fifo.wptr==fifo.rptr ) return -1; // empty
	rd = fifo.buffer[fifo.rptr];
	fifo.rptr = NEXT( fifo.rptr );
	return rd;
}

void RS485_Init(void)
{
	FifoInit();
	BSP_RS485Init();
	BSP_RS485DirTx();
}

void RS485_Task(void)
{
	int ch;
	if( BSP_RS485TxBusy() ) return;	//	busy
	ch = FifoRead();
	if( ch<0 ) return;	//	empty
	BSP_RS485SendData(ch);
}

void RS485_Putc(uint8_t c)
{
	FifoWrite(c);
}

void RS485_Putb(uint8_t * buf, int len)
{
	int i;
	for(i=0;i<len;i++) FifoWrite(buf[i]);
}

void RS485_Puts(uint8_t * str)
{
	while( *str ){
		FifoWrite(*str);
		str ++;
	}
}

