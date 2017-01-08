#include "hwio.h"
#include "timer.h"
#include "bsp.h"
#include <stdio.h>

#define		INPUT_PINS	9

Timer_t hwt;
HWIO_t hwio;
uint8_t his[INPUT_PINS];
uint32_t adc_sum[2];
uint32_t adc_cnt;

void HWIO_Init(void)
{
	uint8_t i;
	for(i=0;i<INPUT_PINS;i++) his[i] = 0xff;
	adc_cnt = 0;
	hwt.StartTime = 0;
	hwt.CycleTime = 1;
	Timer_Load(&hwt);
}

void HWIO_Task(void)
{
	//	check timer
	if( hwt.Updated==0 ) return;
	hwt.Updated = 0;

	//	digit inputs
	{
		uint8_t i;
		uint16_t msk = GPIO_Pin_5;
		uint16_t idr = GPIOC->IDR;
		for(i=0;i<INPUT_PINS;i++){
			his[i] <<= 1;
			if( idr & msk ) his[i] |= 1;
			if( his[i]==0x80 ) hwio.in |= msk;
			if( his[i]==0x7F ) hwio.in &= ~msk;
			msk <<= 1;
		}
	}

	//	digit outputs
	{
		uint32_t set, clr;
		//	PA0, PA4, PA5
		set = clr = 0;
		if( hwio.out & HWIO_OBIT_BEEPER ) set |= GPIO_Pin_0; else clr |= GPIO_Pin_0;	//	BEEPER, PA0, HA
		if( hwio.out & HWIO_OBIT_YSB )    clr |= GPIO_Pin_4; else set |= GPIO_Pin_4;	//	YSB, PA4, LA
		if( hwio.out & HWIO_OBIT_POWER )  clr |= GPIO_Pin_5; else set |= GPIO_Pin_5;	//	POWER, PA5, LA
		GPIOA->BSRR = (clr<<16)|set;
		//	PB4~7, PB12~15
		set = clr = 0;
		if( hwio.out & HWIO_OBIT_QS4 ) clr |= GPIO_Pin_4;  else set |= GPIO_Pin_4;	//	QS4, PA4,  LA
		if( hwio.out & HWIO_OBIT_QS3 ) clr |= GPIO_Pin_5;  else set |= GPIO_Pin_5;	//	QS3, PA5,  LA
		if( hwio.out & HWIO_OBIT_QS2 ) clr |= GPIO_Pin_6;  else set |= GPIO_Pin_6;	//	QS2, PA6,  LA
		if( hwio.out & HWIO_OBIT_QS1 ) clr |= GPIO_Pin_7;  else set |= GPIO_Pin_7;	//	QS1, PA7,  LA
		if( hwio.out & HWIO_OBIT_ZZ4 ) clr |= GPIO_Pin_12; else set |= GPIO_Pin_12;	//	ZZ4, PA12, LA
		if( hwio.out & HWIO_OBIT_ZZ3 ) clr |= GPIO_Pin_13; else set |= GPIO_Pin_13;	//	ZZ3, PA13, LA
		if( hwio.out & HWIO_OBIT_ZZ2 ) clr |= GPIO_Pin_14; else set |= GPIO_Pin_14;	//	ZZ2, PA14, LA
		if( hwio.out & HWIO_OBIT_ZZ1 ) clr |= GPIO_Pin_15; else set |= GPIO_Pin_15;	//	ZZ1, PA15, LA
		GPIOB->BSRR = (clr<<16)|set;
	}

	//	analog inputs
	adc_sum[0] += BSP_ADCRes[0];
	adc_sum[1] += BSP_ADCRes[1];
	adc_cnt ++;
	if( adc_cnt>=32 ){
		hwio.adc[0] = adc_sum[0]/2;		//	16 bits
		hwio.adc[1] = adc_sum[1]/2;
		adc_cnt = 0;
		adc_sum[0] = 0;
		adc_sum[1] = 0;
	}

	//	output
	if(0){
		static uint8_t cnt = 0;
		static HWIO_t io = { 0,0,{0,0}};
		if( cnt==0 ){
			if( io.in!=hwio.in || io.out!=hwio.out || io.adc[0]!=hwio.adc[0] || io.adc[1]!=hwio.adc[1] ){
				printf("hwio: %Xh, %Xh; %d, %d.\r\n", hwio.in, hwio.out, hwio.adc[0], hwio.adc[1]);
				io.in = hwio.in;
				io.out = hwio.out;
				io.adc[0] = hwio.adc[0];
				io.adc[1] = hwio.adc[1];
			}
		}
		if( cnt<10 ) cnt ++; else cnt = 0;
	}

}

