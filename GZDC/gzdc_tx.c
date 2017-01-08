#include "gzdc.h"
#include <stdio.h>

#define TX_8(P1) DC_UartSendChar((P1)&0xFF)  //发送单个字节
#define TX_8N(P,N) SendNU8((uint8 *)P,N)  //发送N个字节
#define TX_16(P1) TX_8((P1)>>8);TX_8(P1)  //发送16位整数
#define TX_16N(P,N) SendNU16((uint16 *)P,N)  //发送N个16位整数
#define TX_32(P1) TX_16((P1)>>16);TX_16((P1)&0xFFFF)  //发送32位整数

#define SEND_DATA(P) SendChar(P)
#define BEGIN_CMD() TX_8(0XEE)
#define END_CMD() TX_32(0XFFFCFFFF)

uint8_t DC_TX_Buffer[256];

int DC_TX_Put8(uint8_t b, int i)
{
	DC_TX_Buffer[i++] = b;
	return i;
}

int DC_TX_Put16(uint16_t w, int i)
{
	DC_TX_Buffer[i++] = w>>8;
	DC_TX_Buffer[i++] = w;
	return i;
}

int DC_TX_PutBuf(char * buf, int len, int i)
{
	int j;
	for(j=0;j<len;j++) DC_TX_Buffer[i++] = buf[j];
	return i;
}

void DC_TX_SetScreen(uint16_t screen_id)
{
printf("%s(%d)\r\n", __func__, screen_id);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x00);
	TX_16(screen_id);
	END_CMD();
}

void DC_TX_GetScreen(void)
{
printf("%s()\r\n", __func__);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x01);
	END_CMD();
}

void DC_TX_SetIcon(uint16_t screen_id, uint16_t control_id, uint8_t frame_id)
{
printf("%s(%d,%d,%d)\r\n", __func__, screen_id, control_id, frame_id);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x23);
	TX_16(screen_id);
	TX_16(control_id);
	TX_8(frame_id);
	END_CMD();
}

void DC_TX_SetButton(uint16_t screen_id, uint16_t control_id, uint8_t state)
{
printf("%s(%d,%d,%d)\r\n", __func__, screen_id, control_id, state);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x10);
	TX_16(screen_id);
	TX_16(control_id);
	TX_8(state);
	END_CMD();
}

void DC_TX_SetVisible(uint16_t screen_id, uint16_t control_id, uint8_t visible)
{
printf("%s(%d,%d,%d)\r\n", __func__, screen_id, control_id, visible);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x03);
	TX_16(screen_id);
	TX_16(control_id);
	TX_8(visible);
	END_CMD();
}

void DC_TX_SetText(uint16_t screen_id, uint16_t control_id, uint8_t len)
{
	int i;
printf("%s(%d,%d,%d)\r\n", __func__, screen_id, control_id, len);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x10);
	TX_16(screen_id);
	TX_16(control_id);
	for(i=0;i<len;i++) TX_8(DC_TX_Buffer[i]);
	END_CMD();
}

void DC_TX_SetFGColor(uint16_t screen_id, uint16_t control_id, uint16_t color)
{
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x19);
	TX_16(screen_id);
	TX_16(control_id);
	TX_16(color);
	END_CMD();
}

void DC_TX_PlayGif(uint16_t screen_id, uint16_t control_id)
{
printf("%s(%d,%d)\r\n", __func__, screen_id, control_id);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x20);
	TX_16(screen_id);
	TX_16(control_id);
	END_CMD();
}

void DC_TX_StopGif(uint16_t screen_id, uint16_t control_id)
{
printf("%s(%d,%d)\r\n", __func__, screen_id, control_id);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x21);
	TX_16(screen_id);
	TX_16(control_id);
	END_CMD();
}

void DC_TX_MultiSet(uint16_t screen_id, int len)
{
	int i;
printf("%s(%d,%d)\r\n", __func__, screen_id, len);
	BEGIN_CMD();
	TX_8(0xB1);
	TX_8(0x12);
	TX_16(screen_id);
	for(i=0;i<len;i++) TX_8(DC_TX_Buffer[i]);
	END_CMD();
}

void DC_TX_WriteFlash(uint16_t addr, int len)
{
	int i;
printf("%s(%d,%d)\r\n", __func__, addr, len);
	BEGIN_CMD();
	TX_8(0x87);
	TX_32(addr);
	for(i=0;i<len;i++) TX_8(DC_TX_Buffer[i]);
	END_CMD();
}

void DC_TX_ReadFlash(uint16_t addr, int len)
{
printf("%s(%d,%d)\r\n", __func__, addr, len);
	BEGIN_CMD();
	TX_8(0x88);
	TX_32(addr);
	TX_16(len);
	END_CMD();
}
