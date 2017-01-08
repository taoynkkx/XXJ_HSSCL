//	使用15us定时器中断，扫描操作通信接口
#include "ds18b20.h"
#include "stm32f10x.h"
#include <stdio.h>

#define		DQ_LOW()		GPIOA->BRR  = GPIO_Pin_15
#define		DQ_HIGH()		GPIOA->BSRR = GPIO_Pin_15
#define		DQ_IS_HIGH()	(GPIOA->IDR & GPIO_Pin_15)
#define		DQ_DATA()		(GPIOA->IDR&GPIO_Pin_15)?1:0

#define		CMD_SEARCH_ROM				0xF0
#define		CMD_READ_ROM				0x33
#define		CMD_MATCH_ROM				0x55
#define		CMD_SKIP_ROM				0xCC
#define		CMD_ALARM_SEARCH			0xEC
#define		CMD_CONVERT_TEMP			0x44
#define		CMD_WRITE_SCRATCH_PAD		0x4E
#define		CMD_READ_SCRATCH_PAD		0xBE
#define		CMD_COPY_SCRATCH_PAD		0x48
#define		CMD_RECALL_EEPROM			0xB8
#define		CMD_READ_POWER_SUPPLY		0xB4

void delay_init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseStructure.TIM_Period = 0xffff;
	TIM_TimeBaseStructure.TIM_Prescaler = 72-1;  // 1MHz - 1us
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 
	TIM_Cmd(TIM3, ENABLE);
}

void delay_us(uint16_t us)
{
	TIM3->CNT = 0;
	while( TIM3->CNT < us );
}

void DS_Init(void)
{
	delay_init();
}

int ds_init(void)
{
	int i;

	// out low 480us+
	DQ_LOW();
	delay_us(550);
	DQ_HIGH();

	// wait 15~60us
	delay_us(50);

	// check presense
	if( DQ_IS_HIGH() ){
		printf("ERROR: sensor not present!\r\n");
		return -1; // not present
	}

	// wait DQ rise
	for(i=0;i<255;i++){
		if( DQ_IS_HIGH() ) break; // rised
		delay_us(50);
	}

	// wait DQ stable
	delay_us(50);

	// rised
	if( i<255 ) return 0;

	// not rised
	printf("ERROR: sensor init failed!\r\n");
	return -1;
}

void ds_write_bit(uint8_t b)
{
	// start
	DQ_LOW();
	delay_us(10);

	// data
	if( b&0x01 ) DQ_HIGH(); else DQ_LOW();
	delay_us(60);

	// stop
	DQ_HIGH();
	delay_us(10);
}

uint8_t ds_read_bit(void)
{
	uint8_t b;

	// start
	DQ_LOW();
	delay_us(10);

	// wait
	DQ_HIGH();
	delay_us(10);

	// read data
	b = DQ_DATA();

	// stop
	delay_us(60);

	// ok
	return b;
}

void ds_write_byte(uint8_t b)
{
	u8 i;
	for(i=0;i<8;i++){
		ds_write_bit(b);
		b >>= 1;
	}
}

uint8_t ds_read_byte(void)
{
	u8 i,b;
	for(i=0,b=0;i<8;i++){
		b >>= 1;
		if( ds_read_bit() ) b |= 0x80;
	}
	return b;
}

union{
    struct{
        uint8_t tl;
        uint8_t th;
    }ba;
    uint16_t u;
    int16_t s;
}temp;

int DS_ReadTemp(void)
{
	int res;
	//	init seq	//	600us+600us
	res = ds_init();
	if( res<0 ) return res;
	//	write CCh	//	skip rom	60us*8
	ds_write_byte(0xCC);
	//	write BEh	//	read rom	60us*8
	ds_write_byte(0xBE);
	//	read  TL	//	60us*8
	temp.ba.tl = ds_read_byte();
	//	read  TH	//	60us*8
	temp.ba.th = ds_read_byte();

	//	init seq	//	600us+600us
	res = ds_init();
	if( res<0 ) return res;
	//	write CCh	//	skip rom	60us*8
	ds_write_byte(0xCC);
	//	write 44h	//	start convert	60us*8
	ds_write_byte(0x44);

	res = temp.s * 10 / 16;

//printf("Read Temp: %d.%d\r\n", res/10, res%10);
	return res;
}


