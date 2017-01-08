#include "hsscl.h"
#include <stdio.h>
#include "timer.h"
#include "hwio.h"
#include "gzdc.h"
#include <string.h>
#include "hsio.h"
#include "rs485.h"
#include "bsp.h"

HSCFG_t hscfg;
Timer_t t100;	//  100ms
Timer_t t1s;	//	1s
Timer_t t1m;	//	1m
Timer_t p2t;	//	1s
uint32_t runtime;

static Timer_t opt;	//	1s

//	内部逻辑
void HS_Logic(void)
{
	//	运行总时间
	if( t1m.Updated ){
		t1m.Updated = 0;
		runtime ++;
		HS_SaveRuntime(runtime);
	}
	//	电解电源：液位高才能打开，否则关闭
	if( (hwio.out&HWIO_OBIT_POWER) && !(hwio.in&HWIO_IBIT_YWG) ){
		hwio.out &= ~HWIO_OBIT_POWER;
		printf("HS_Logic: POWER OFF for not YWG\r\n");
	}
}

void HS_Idle(void)
{
	HS_Logic();		//	内部逻辑优先	
	HSIO_Task();	//	内部输出优先

	//	底层
	Timer_Task();
	HWIO_Task();
	RS485_Task();

	//	输出
	if( opt.Updated ){
		static uint32_t cnt = 0;
		opt.Updated = 0;
		printf("[%u] %Xh, %Xh, %Xh; %d, %d, %d.\r\n", 
			cnt++,
			hwio.in, hwio.out, hsio.out,
			hsio.volt, hsio.curr, hsio.temp);

		//	输出485
		RS485_Putc(0x55);
		RS485_Putc(0x55);
		RS485_Putc(0x55);
		RS485_Putc(0x55);
		RS485_Putc(hwio.in>>8);
		RS485_Putc(hwio.in);
		RS485_Putc(hwio.out>>8);
		RS485_Putc(hwio.out);
		RS485_Putc(hsio.out>>8);
		RS485_Putc(hsio.out);
		RS485_Putc(hsio.volt>>8);
		RS485_Putc(hsio.volt);
		RS485_Putc(hsio.curr>>8);
		RS485_Putc(hsio.curr);
		RS485_Putc(hsio.temp>>8);
		RS485_Putc(hsio.temp);
		RS485_Putc(0xAA);
		RS485_Putc(0xAA);
		RS485_Putc(0xAA);
		RS485_Putc(0xAA);
	}
}

void HS_Main(void)
{
	int curr = 0;
	int next = 0;

	printf("%s()\r\n", __func__);

	Timer_Init();
	HWIO_Init();
	RS485_Init();
	DC_Init();
	HSIO_Init();

	//	100ms 定时器
	t100.StartTime = 0;
	t100.CycleTime = 10;
	Timer_Load(&t100);

	//	1s 定时器
	t1s.StartTime = 0;
	t1s.CycleTime = 100;
	Timer_Load(&t1s);

	//	1s 定时器
	opt.StartTime = 10;
	opt.CycleTime = 100;
	Timer_Load(&opt);

	//	1s 定时器
	p2t.StartTime = 20;
	p2t.CycleTime = 100;
	Timer_Load(&p2t);

	//	1m 定时器
	t1m.StartTime = 0;
	t1m.CycleTime = 100*60;
	Timer_Load(&t1m);

	HS_LoadConfig();
	runtime = HS_LoadRuntime();

	while(1){

		switch( curr ){
			case 0: next = HS_Page_0(); break;
			case 1: next = HS_Page_1(); break;
			case 2: next = HS_Page_2(); break;
			case 3: next = HS_Page_3(); break;
			case 4: next = HS_Page_4(); break;
			case 5: next = HS_Page_5(); break;
			default: next = 0;
		}

		if( next!=curr ){
			printf("switch screen: %d -> %d\r\n", curr, next);
			curr = next;
		}

	}
}

int HS_WaitDownCmd(void)
{
	int rxcnt;
	//	等待输入或超时
	t100.Updated = 0;
	while( t100.Updated==0 ){
		HS_Idle();
		rxcnt = DC_RX_FindCmd();
		if( rxcnt>0 ) break;	//	收到输入
	}
	if( t100.Updated ){
		printf("ERROR: wait down cmd time out!\r\n");
		return -1;	//	超时
	}
	//	返回长度
	return rxcnt;
}

void HS_ConfirmScreen(uint16_t screen_id)
{
//	int i;
	int rxcnt;

	printf("%s(%d)\r\n", __func__, screen_id);

	//	设置并确认画面0
	for(;;){

		//	发送设置命令
//		printf("set screen %d\r\n", screen_id);
		DC_TX_SetScreen(screen_id);

		//	延时等待生效
		HS_Delay100MS(2);

		//	发送查询命令
//		printf("get screen\r\n");
		DC_RX_ClearCache();
		DC_TX_GetScreen();

		//	等待返回信息
		rxcnt = HS_WaitDownCmd();
		if( rxcnt<0 ) continue;		//	超时、重试

		//	判断返回信息
//		printf("check data [%d] : ", rxcnt);
//		for(i=0;i<rxcnt;i++) printf("%02x ", DC_RX_Buffer[i]);
//		printf("\r\n");
		if( RX_CMD_TYPE==0xB1 && RX_CTRL_MSG==0x01 &&
			RX_SCREEN_ID==screen_id ){
			break;	//	通过
		}

	}

}

void HS_DelaySec(uint32_t sec)
{
	while( sec>0 ){
		printf("delay %u\r\n", sec);
		t1s.Updated = 0;
		while( t1s.Updated==0 ) HS_Idle();
		sec --;
	}
}

void HS_Delay100MS(uint32_t hms)
{
	while( hms>0 ){
		t100.Updated = 0;
		while( t100.Updated==0 ) HS_Idle();
		hms --;		
	}
}

#define		SAVE_IN_EEPROM

void HS_SaveConfig(void)
{
//	int i;
	printf("%s()\r\n", __func__);

	//	make verification
	hscfg.Magic = 0x55AA;
	hscfg.Check = 0;
	hscfg.Check -= hscfg.Magic;
	hscfg.Check -= hscfg.Volt;
	hscfg.Check -= hscfg.Curr;
	hscfg.Check -= hscfg.Temp;
	hscfg.Check -= hscfg.Time;
	hscfg.Check -= hscfg.Mode;

	//	write to flash
	memcpy(DC_TX_Buffer, &hscfg, sizeof(hscfg));

#ifdef	SAVE_IN_EEPROM
	EEP_Write(DC_TX_Buffer, sizeof(hscfg), 0x80, ADDR_24C08);
#else
	DC_TX_WriteFlash(0,sizeof(hscfg));
#endif

//	for(i=0;i<sizeof(hscfg);i++) printf("%x ", DC_TX_Buffer[i]);
//	printf("\r\n");
}

void HS_LoadConfig(void)
{
//	int i;
	int r;
	printf("%s()\r\n", __func__);

	//	read from flash
#ifdef	SAVE_IN_EEPROM
	r = EEP_Read(DC_RX_Buffer, sizeof(hscfg), 0x80, ADDR_24C08);
	if( r==ENABLE ){
		memcpy(&hscfg, DC_RX_Buffer, sizeof(hscfg));
#else
	DC_TX_ReadFlash(0,sizeof(hscfg));
	r = HS_WaitDownCmd();
//	for(i=0;i<r;i++) printf("%x ", DC_RX_Buffer[i]);
//	printf("\r\n");
	if( r==6+sizeof(hscfg) && RX_CMD_TYPE==0x0B ){
		memcpy(&hscfg, &DC_RX_Buffer[2], sizeof(hscfg));
#endif
		printf("-- magic: %Xh\r\n", hscfg.Magic);
		printf("-- volt:  %d V\r\n", hscfg.Volt);
		printf("-- curr:  %d A\r\n", hscfg.Curr);
		printf("-- temp:  %d C\r\n", hscfg.Temp);
		printf("-- time:  %d Min\r\n", hscfg.Time);
		printf("-- mode:  %Xh\r\n", hscfg.Mode);
		printf("-- check: %Xh\r\n", hscfg.Check);
		hscfg.Check += hscfg.Magic;
		hscfg.Check += hscfg.Volt;
		hscfg.Check += hscfg.Curr;
		hscfg.Check += hscfg.Temp;
		hscfg.Check += hscfg.Time;
		hscfg.Check += hscfg.Mode;
		printf("==> %Xh\r\n", hscfg.Check);
		if( hscfg.Check==0 ){
			printf("verify pass\r\n");
			return;
		}
	}

	//	初始化存储
	hscfg.Volt = 20;
	hscfg.Curr = 300;
	hscfg.Temp = 100;
	hscfg.Time = 999;
	hscfg.Mode = 0;
	HS_SaveConfig();
//	HS_SaveRuntime(0);
//	HS_SaveSST();
}

void HS_SaveRuntime(uint32_t rt)
{
	printf("%s(%d)\r\n", __func__, rt);
	DC_TX_Buffer[0] = rt>>24;
	DC_TX_Buffer[1] = rt>>16;
	DC_TX_Buffer[2] = rt>>8;
	DC_TX_Buffer[3] = rt;
	DC_TX_Buffer[4] = ~DC_TX_Buffer[0];
	DC_TX_Buffer[5] = ~DC_TX_Buffer[1];
	DC_TX_Buffer[6] = ~DC_TX_Buffer[2];
	DC_TX_Buffer[7] = ~DC_TX_Buffer[3];
	EEP_Write(DC_TX_Buffer, 8, 0x40, ADDR_24C08);
}

uint32_t HS_LoadRuntime(void)
{
	if( EEP_Read(DC_RX_Buffer, 8, 0x40, ADDR_24C08)==ENABLE ){
		if( DC_RX_Buffer[0] + DC_RX_Buffer[4] == 0xFF &&
			DC_RX_Buffer[1] + DC_RX_Buffer[5] == 0xFF &&
			DC_RX_Buffer[2] + DC_RX_Buffer[6] == 0xFF &&
			DC_RX_Buffer[3] + DC_RX_Buffer[7] == 0xFF )
		{
			uint32_t rt = 0;
			rt <<= 8; rt = DC_RX_Buffer[0];
			rt <<= 8; rt = DC_RX_Buffer[1];
			rt <<= 8; rt = DC_RX_Buffer[2];
			rt <<= 8; rt = DC_RX_Buffer[3];

			printf("%s()=%d\r\n", __func__, rt);
			return rt;
		}
	}
	HS_SaveRuntime(0);
	return 0;
}

void HS_SaveStepStateTime(uint8_t step, uint8_t state, uint32_t time)
{
	printf("%s(%d,%d,%d)\r\n", __func__,step,state,time);
	DC_TX_Buffer[0] = step;
	DC_TX_Buffer[1] = state;
	DC_TX_Buffer[2] = time>>24;
	DC_TX_Buffer[3] = time>>16;
	DC_TX_Buffer[4] = time>>8;
	DC_TX_Buffer[5] = time;
#ifdef	SAVE_IN_EEPROM
	EEP_Write(DC_TX_Buffer, 6, 0x00, ADDR_24C08);
#else
	DC_TX_WriteFlash(0x100,6);
#endif
}

void HS_LoadStepStateTime(uint8_t * step, uint8_t * state, uint32_t * time)
{
	uint32_t tmp;
#ifdef	SAVE_IN_EEPROM
	while( EEP_Read(&DC_RX_Buffer[2], 6, 0x00, ADDR_24C08)==DISABLE );
#else
	do
	{
		DC_TX_ReadFlash(0x100,6);
	}
	while(HS_WaitDownCmd()<0);
#endif

	*step = DC_RX_Buffer[2];
	*state = DC_RX_Buffer[3];
	tmp = DC_RX_Buffer[4];
	tmp <<= 8; tmp |= DC_RX_Buffer[5];
	tmp <<= 8; tmp |= DC_RX_Buffer[6];
	tmp <<= 8; tmp |= DC_RX_Buffer[7];
	*time = tmp;
	printf("%s(%d,%d,%d)\r\n", __func__,*step,*state,*time);
}

