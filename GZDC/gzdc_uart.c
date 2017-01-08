#include "gzdc.h"
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
RxFifo_t;

static RxFifo_t fifo;

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

void DC_UartInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;	
	USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

	FifoInit();

    /* Enable USART Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 6;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	
	USART_Init(USART2, &USART_InitStructure);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);
}

void DC_UartSendChar(uint8_t ch)
{
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	USART_SendData(USART2,ch);
	while(USART_GetFlagStatus(USART2, USART_FLAG_TC ) == RESET);
}

void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET){
		uint16_t rx = USART_ReceiveData(USART2);
		FifoWrite(rx);
	}
}

int DC_UartGetChar(void)
{
	int res;
	res = FifoRead();
if( res>=0 ) printf("%X^", res);
	return res;
}

