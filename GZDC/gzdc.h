#ifndef  __GZDC_H
#define  __GZDC_H

#include <stdint.h>

extern void DC_UartInit(void);
extern void DC_UartSendChar(uint8_t ch);
extern int  DC_UartGetChar(void);

extern void DC_Init(void);
extern void DC_Task(void);

#define		RX_CMD_TYPE		DC_RX_Buffer[1]
#define		RX_CTRL_MSG		DC_RX_Buffer[2]
#define		RX_SCREEN_ID	((DC_RX_Buffer[3]<<8)|DC_RX_Buffer[4])
#define		RX_CONTROL_ID	((DC_RX_Buffer[5]<<8)|DC_RX_Buffer[6])
#define		RX_CONTROL_TYPE	DC_RX_Buffer[7]
#define		RX_DATA			DC_RX_Buffer[8]
extern uint8_t DC_RX_Buffer[256];
extern void DC_RX_ClearCache(void);
extern int  DC_RX_FindCmd(void);

extern void DC_TX_SetScreen(uint16_t screen_id);
extern void DC_TX_GetScreen(void);
extern void DC_TX_SetVisible(uint16_t screen_id, uint16_t control_id, uint8_t visible);
extern void DC_TX_SetIcon(uint16_t screen_id, uint16_t control_id, uint8_t frame_id);
extern void DC_TX_SetButton(uint16_t screen_id, uint16_t control_id, uint8_t state);
extern void DC_TX_PlayGif(uint16_t screen_id, uint16_t control_id);
extern void DC_TX_StopGif(uint16_t screen_id, uint16_t control_id);
extern void DC_TX_SetText(uint16_t screen_id, uint16_t control_id, uint8_t len);
extern void DC_TX_SetFGColor(uint16_t screen_id, uint16_t control_id, uint16_t color);

extern uint8_t DC_TX_Buffer[256];
extern int DC_TX_Put8(uint8_t b, int i);
extern int DC_TX_Put16(uint16_t w, int i);
extern int DC_TX_PutBuf(char * buf, int len, int i);
extern void DC_TX_MultiSet(uint16_t screen_id, int len);

extern void DC_TX_WriteFlash(uint16_t addr, int len);
extern void DC_TX_ReadFlash(uint16_t addr, int len);

#endif // __GZDC_H

