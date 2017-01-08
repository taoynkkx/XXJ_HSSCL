#ifndef  __HSSCL_H
#define  __HSSCL_H

#include <stdint.h>
#include "timer.h"

typedef struct
{
	uint16_t Magic;		//	0x55AA
	uint16_t Volt;		//	0~20V
	uint16_t Curr;		//	0~300A
	uint16_t Temp;		//	5~100C
	uint16_t Time;		//	0~999M
#define		HSCFG_MODE_AUTO		0x0001
	uint16_t Mode;		//	bit0: auto mode
	uint16_t Check;		//	
}
HSCFG_t;

extern HSCFG_t hscfg;
extern Timer_t t100;	//  100ms
extern Timer_t t1s;		//	1s
extern Timer_t p2t;		//	1s
extern uint32_t runtime;

extern void HS_Main(void);
extern void HS_Idle(void);

extern void HS_SaveConfig(void);
extern void HS_LoadConfig(void);
extern void HS_SaveRuntime(uint32_t rt);
extern uint32_t HS_LoadRuntime(void);
extern void HS_SaveStepStateTime(uint8_t step, uint8_t state, uint32_t time);
extern void HS_LoadStepStateTime(uint8_t * step, uint8_t * state, uint32_t * time);

extern void HS_DelaySec(uint32_t);
extern void HS_Delay100MS(uint32_t);
extern int  HS_WaitDownCmd(void);
extern void HS_ConfirmScreen(uint16_t);

extern int HS_Page_0(void);
extern int HS_Page_1(void);
extern int HS_Page_2(void);
extern int HS_Page_3(void);
extern int HS_Page_4(void);
extern int HS_Page_5(void);

#endif // __HSSCL_H

