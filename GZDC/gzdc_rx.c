#include "gzdc.h"
#include <stdio.h>

#define CMD_HEAD 0XEE  //帧头
#define CMD_TAIL 0XFFFCFFFF //帧尾

#define	RXS_WAIT_HEAD	0
#define	RXS_WAIT_DATA	1
static uint8_t rxs = RXS_WAIT_HEAD;
static uint32_t tail;
static uint8_t idx;
uint8_t DC_RX_Buffer[256];

void DC_RX_ClearCache(void)
{
	//	丢掉缓存数据
	while( DC_UartGetChar()>=0 );
	//	初始状态
	rxs = RXS_WAIT_HEAD;
}

int DC_RX_FindCmd(void)
{
	int rx = DC_UartGetChar();
	if( rx<0 ) return -1;

	//	wait head
	if( rxs==RXS_WAIT_HEAD ){
		//	第一个必须是帧头，否则丢弃跳过
		if( rx!=CMD_HEAD ){
			printf("ERROR: rx[%Xh] is not head.\r\n", rx);
			return -1;
		}
		//	接收初始化
		DC_RX_Buffer[0] = rx;
		idx = 1;
		rxs = RXS_WAIT_DATA;
		tail = 0;
		//	返回
		return -1;
	}

	//	save data
	if( idx>255 ){	//	数组溢出
		rxs = RXS_WAIT_HEAD;	//	重新开始
		printf("ERROR: rx buffer overflow.\r\n");
		return -1;		
	}
	DC_RX_Buffer[idx] = rx;	//	保存
	idx ++;	//	累加

	//	check tail
	tail <<= 8;
	tail |= rx;
	if( tail!=CMD_TAIL ) return -1;	//	继续

	//	完成
	rxs = RXS_WAIT_HEAD;

printf("%s() = %d\r\n", __func__, idx);

	//	返回数组大小
	return idx;	//	不算tail
}

