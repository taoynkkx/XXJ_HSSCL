#include "hsio.h"
#include "hsscl.h"
#include "hwio.h"
#include "timer.h"
#include "ds18b20.h"
#include <stdio.h>

HSIO_t hsio;
Timer_t t3s;

static void Output(void)
{
	//	清水阀
	if( hsio.out & HSIO_OBIT_QS ){
		hwio.out &= ~(HWIO_OBIT_QS1|HWIO_OBIT_QS2|HWIO_OBIT_QS3|HWIO_OBIT_QS4);				
		hwio.out |= (HWIO_OBIT_QS1|HWIO_OBIT_QS4);
	}else{
		hwio.out &= ~(HWIO_OBIT_QS1|HWIO_OBIT_QS2|HWIO_OBIT_QS3|HWIO_OBIT_QS4);				
		hwio.out |= (HWIO_OBIT_QS2|HWIO_OBIT_QS3);
	}
	//	转存阀
	if( hsio.out & HSIO_OBIT_ZZ ){
		hwio.out &= ~(HWIO_OBIT_ZZ1|HWIO_OBIT_ZZ2|HWIO_OBIT_ZZ3|HWIO_OBIT_ZZ4);				
		hwio.out |= (HWIO_OBIT_ZZ1|HWIO_OBIT_ZZ4);
	}else{
		hwio.out &= ~(HWIO_OBIT_ZZ1|HWIO_OBIT_ZZ2|HWIO_OBIT_ZZ3|HWIO_OBIT_ZZ4);				
		hwio.out |= (HWIO_OBIT_ZZ2|HWIO_OBIT_ZZ3);
	}
}

void HSIO_Init(void)
{
	hsio.out = 0;
	hsio.temp = -2730;	//	初始化为绝对0度
	Output();
	DS_Init();
	t3s.StartTime = 50;
	t3s.CycleTime = 100;
	Timer_Load(&t3s);
}

void HSIO_Task(void)
{
	//	输出
	{
		static uint32_t out = (uint32_t)-1;
		if( out!=hsio.out ){
			//	状态输出
			Output();
			//	保存新状态
			out = hsio.out;
		}
	}
	//	电压
	{
		static uint16_t adc = (uint16_t)-1;
		if( adc!=hwio.adc[1] ){
			adc = hwio.adc[1];
			hsio.volt = adc * hscfg.Volt / 0x10000;
		}
	}
	//	电流
	{
		static uint16_t adc = (uint16_t)-1;
		if( adc!=hwio.adc[0] ){
			adc = hwio.adc[0];
			hsio.curr = adc * hscfg.Curr / 0x10000;
		}
	}
	//	温度
	{
		if( t3s.Updated ){
			int tmp;
			t3s.Updated = 0;
			tmp = DS_ReadTemp();
			//	-273度表示初始化
			if( hsio.temp == -2730 ){
				//	正常范围内：-40~85
				if( tmp > -400 && tmp < 850 ){
					hsio.temp = tmp;
				}
				//	超出范围
				else{
					printf("Error temp TOO high/low: %d\r\n", tmp);
				}
			}
			//	变化幅度太大
			else if( tmp<hsio.temp-100 || tmp>hsio.temp+100 ){
				printf("Error Temp Change: %d\r\n", tmp);
			}
			//	更新温度
			else{
				hsio.temp = tmp;
			}
		}
	}
}

