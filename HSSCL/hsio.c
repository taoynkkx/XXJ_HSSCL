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
	//	��ˮ��
	if( hsio.out & HSIO_OBIT_QS ){
		hwio.out &= ~(HWIO_OBIT_QS1|HWIO_OBIT_QS2|HWIO_OBIT_QS3|HWIO_OBIT_QS4);				
		hwio.out |= (HWIO_OBIT_QS1|HWIO_OBIT_QS4);
	}else{
		hwio.out &= ~(HWIO_OBIT_QS1|HWIO_OBIT_QS2|HWIO_OBIT_QS3|HWIO_OBIT_QS4);				
		hwio.out |= (HWIO_OBIT_QS2|HWIO_OBIT_QS3);
	}
	//	ת�淧
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
	hsio.temp = -2730;	//	��ʼ��Ϊ����0��
	Output();
	DS_Init();
	t3s.StartTime = 50;
	t3s.CycleTime = 100;
	Timer_Load(&t3s);
}

void HSIO_Task(void)
{
	//	���
	{
		static uint32_t out = (uint32_t)-1;
		if( out!=hsio.out ){
			//	״̬���
			Output();
			//	������״̬
			out = hsio.out;
		}
	}
	//	��ѹ
	{
		static uint16_t adc = (uint16_t)-1;
		if( adc!=hwio.adc[1] ){
			adc = hwio.adc[1];
			hsio.volt = adc * hscfg.Volt / 0x10000;
		}
	}
	//	����
	{
		static uint16_t adc = (uint16_t)-1;
		if( adc!=hwio.adc[0] ){
			adc = hwio.adc[0];
			hsio.curr = adc * hscfg.Curr / 0x10000;
		}
	}
	//	�¶�
	{
		if( t3s.Updated ){
			int tmp;
			t3s.Updated = 0;
			tmp = DS_ReadTemp();
			//	-273�ȱ�ʾ��ʼ��
			if( hsio.temp == -2730 ){
				//	������Χ�ڣ�-40~85
				if( tmp > -400 && tmp < 850 ){
					hsio.temp = tmp;
				}
				//	������Χ
				else{
					printf("Error temp TOO high/low: %d\r\n", tmp);
				}
			}
			//	�仯����̫��
			else if( tmp<hsio.temp-100 || tmp>hsio.temp+100 ){
				printf("Error Temp Change: %d\r\n", tmp);
			}
			//	�����¶�
			else{
				hsio.temp = tmp;
			}
		}
	}
}

