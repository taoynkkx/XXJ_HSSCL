#include "gzdc.h"
#include <stdio.h>

#define CMD_HEAD 0XEE  //֡ͷ
#define CMD_TAIL 0XFFFCFFFF //֡β

#define	RXS_WAIT_HEAD	0
#define	RXS_WAIT_DATA	1
static uint8_t rxs = RXS_WAIT_HEAD;
static uint32_t tail;
static uint8_t idx;
uint8_t DC_RX_Buffer[256];

void DC_RX_ClearCache(void)
{
	//	������������
	while( DC_UartGetChar()>=0 );
	//	��ʼ״̬
	rxs = RXS_WAIT_HEAD;
}

int DC_RX_FindCmd(void)
{
	int rx = DC_UartGetChar();
	if( rx<0 ) return -1;

	//	wait head
	if( rxs==RXS_WAIT_HEAD ){
		//	��һ��������֡ͷ������������
		if( rx!=CMD_HEAD ){
			printf("ERROR: rx[%Xh] is not head.\r\n", rx);
			return -1;
		}
		//	���ճ�ʼ��
		DC_RX_Buffer[0] = rx;
		idx = 1;
		rxs = RXS_WAIT_DATA;
		tail = 0;
		//	����
		return -1;
	}

	//	save data
	if( idx>255 ){	//	�������
		rxs = RXS_WAIT_HEAD;	//	���¿�ʼ
		printf("ERROR: rx buffer overflow.\r\n");
		return -1;		
	}
	DC_RX_Buffer[idx] = rx;	//	����
	idx ++;	//	�ۼ�

	//	check tail
	tail <<= 8;
	tail |= rx;
	if( tail!=CMD_TAIL ) return -1;	//	����

	//	���
	rxs = RXS_WAIT_HEAD;

printf("%s() = %d\r\n", __func__, idx);

	//	���������С
	return idx;	//	����tail
}

