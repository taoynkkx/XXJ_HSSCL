// 串口底层初始化和基本操作

#include "bsp.h"

void BSP_RS485DirTx(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_0);
}

void BSP_RS485DirRx(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}

void BSP_RS485Init(void)
{	
	GPIO_InitTypeDef GPIO_InitStructure;	
	USART_InitTypeDef USART_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	//	PB0-EN
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	//	PB10-TX3,PB11-RX3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	//	USART3
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;// | USART_Mode_Rx;
	USART_Init(USART3, &USART_InitStructure);
	USART_Cmd(USART3, ENABLE);
}

int BSP_RS485TxBusy(void)
{
	if( USART_GetFlagStatus(USART3, USART_FLAG_TXE)==RESET ) return 1;
	return 0;
}

void BSP_RS485SendData(int ch)
{
	USART_SendData(USART3, ch);
}

