#include "hsscl.h"
#include <stdio.h>
#include "gzdc.h"
#include "hsio.h"
#include "hwio.h"
#include <string.h>

#define		STEP_STB		0	//	����
#define		STEP_YSB		1	//	��ˮ��
#define		STEP_QSF		2	//	��ˮ��
#define		STEP_PWR		3	//	����Դ
#define		STEP_ZZF		4	//	ת�淧
#define		STEP_ITV		5	//	ѭ����Ъ
#define		STEP_ERR		6	//	������ͣ
#define		STEP_EXIT		9	//	�˳�

#define		COLOR_GREEN		0x0600
#define		COLOR_RED		0xF800
#define		COLOR_ORANGE	0xFB20

static uint32_t step;	//	���в���
static uint32_t state;	//	��״̬
static uint32_t time;	//	����ʱ
static uint32_t stored_step;
static uint32_t stored_state;

static struct{
	uint32_t in;	//	hwio.in
	uint32_t out;	//	hwio.out
	uint32_t hso;	//	hsio.out
	uint32_t step;
	uint32_t volt;
	uint32_t curr;
	int32_t temp;
	uint32_t status;
	uint32_t time;
	uint32_t runtime;
}gs;

//	���״̬����Ҫʱ����
static void Page2Update(void)
{
	//	��ͣ��ť���͡�����ָʾ
	if( gs.step!=step ){
		if( step==STEP_STB ) DC_TX_SetIcon(2,9,0); else DC_TX_SetIcon(2,9,1);
		DC_TX_SetIcon(2,11,step);
		//	����״̬
		gs.step = step;
	}
	//	�����ź�
	if( gs.in!=hwio.in ){
		//	��ˮ����λ
		if( hwio.in & HWIO_IBIT_QSX ) DC_TX_SetVisible(2,14,0); else DC_TX_SetVisible(2,14,1);
		//	ת�淧��λ
		if( hwio.in & HWIO_IBIT_ZZX ) DC_TX_SetVisible(2,16,0); else DC_TX_SetVisible(2,16,1);
		//	Һλָʾ
		if( hwio.in & HWIO_IBIT_YWG ) DC_TX_SetIcon(2,15,7);
		else if( hwio.in & HWIO_IBIT_YWZ ) DC_TX_SetIcon(2,15,2);
		else if( hwio.in & HWIO_IBIT_YWD ) DC_TX_SetIcon(2,15,0);
		else DC_TX_SetIcon(2,15,0xff);
		//	�ⲿ��Դ�����ź�
		if( hwio.in & HWIO_IBIT_ERM ){
//			const char msg[] = "!*��Դ����*!";
//			strcpy((void*)DC_TX_Buffer, (void*)msg);
//			DC_TX_SetText(2,17,strlen(msg));
//			DC_TX_SetFGColor(2,17,COLOR_RED);
		}
		//	����״̬
		gs.in = hwio.in;
	}
	//	����ź�
	if( gs.out!=hwio.out ){
		//	��ˮ��
		if( hwio.out & HWIO_OBIT_YSB ) DC_TX_SetVisible(2,13,1); else DC_TX_SetVisible(2,13,0);
		//	����Դ
		if( hwio.out & HWIO_OBIT_POWER ) DC_TX_SetVisible(2,7,1); else DC_TX_SetVisible(2,7,0);
		//	����״̬
		gs.out = hwio.out;
	}
	//	����ʱ
	if( gs.time!=time ){
		uint32_t m;
		m = time / 60;
		sprintf((void*)DC_TX_Buffer,"%d:%d",m/60,m%60);
		DC_TX_SetText(2,3,strlen((void*)DC_TX_Buffer));
		//	����״̬
		gs.time = time;
	}
	//	��ʱ��(��)
	if( gs.runtime!=runtime ){
		sprintf((void*)DC_TX_Buffer,"%u",runtime/60);	//	Сʱ
		DC_TX_SetText(2,5,strlen((void*)DC_TX_Buffer));
		//	����״̬
		gs.runtime = runtime;
	}
	//	����
	if( gs.curr!=hsio.curr ){
		sprintf((void*)DC_TX_Buffer,"%d", hsio.curr);
		DC_TX_SetText(2,2,strlen((void*)DC_TX_Buffer));
		//	����״̬
		gs.curr = hsio.curr;
	}
	//	��ѹ
	if( gs.volt!=hsio.volt ){
		sprintf((void*)DC_TX_Buffer,"%d", hsio.volt);
		DC_TX_SetText(2,1,strlen((void*)DC_TX_Buffer));
		//	����״̬
		gs.volt = hsio.volt;
	}
	//	�¶�
	if( gs.temp!=hsio.temp ){
		sprintf((void*)DC_TX_Buffer,"%d.%d", hsio.temp/10, hsio.temp%10);
		DC_TX_SetText(2,4,strlen((void*)DC_TX_Buffer));
		//	����״̬
		gs.temp = hsio.temp;
	}
}

//	��ʾ�豸״̬
static void DevState(char * msg, uint16_t color)
{
printf("%s(%s,)\r\n", __func__, msg);
//	sprintf((void*)DC_TX_Buffer, "%d%d%s", step, state, msg);
	DC_TX_Buffer[0] = '0'+step;
	DC_TX_Buffer[1] = '0'+state;
	memcpy(&DC_TX_Buffer[2],msg,strlen(msg));
	DC_TX_SetText(2,17,2+strlen(msg));
	DC_TX_SetFGColor(2,17,color);
}

//	��������
#define		INPUT_RETURN_DOWN		0x01
#define		INPUT_RETURN_UP			0x02
#define		INPUT_START_STOP		0x04
#define		RETURN_DOWN()			DC_TX_SetIcon(2,8,1)
#define		RETURN_UP()				DC_TX_SetIcon(2,8,0)
static uint32_t input;
static void Page2Input(void)
{
	int r;
	input = 0;
	r = DC_RX_FindCmd();
	if( r>0 && RX_CMD_TYPE==0xB1 && RX_CTRL_MSG==0x26 && RX_SCREEN_ID==2 ){
		if( RX_CONTROL_ID==9 ){	//	��ͣ
			if( RX_CONTROL_TYPE==1 ){	//	����
				input |= INPUT_START_STOP;
			}
		}
		if( RX_CONTROL_ID==8 ){	//	����
			if( RX_CONTROL_TYPE==1 ){	//	����
				input |= INPUT_RETURN_DOWN;
			}else if( RX_CONTROL_TYPE==0 ){	//	����
				input |= INPUT_RETURN_UP;
			}
		}
	}
}

//	�����˳�
static int CheckExit(void)
{
	//	�˳�
	if( input & INPUT_RETURN_UP ) return 1;
	//	�ر������豸
	if( input & INPUT_RETURN_DOWN ){
		hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);		//	�ر���ˮ����ת�淧
		hwio.out &= ~(HWIO_OBIT_YSB|HWIO_OBIT_POWER);	//	�ر���ˮ�ú͵���Դ
	}
	//	û���˳�
	return	0;
}

//	ֹͣ
static int CheckStop(void)
{
	if( input & INPUT_START_STOP ){
		state = 0;
		return 1;
	}
	return 0;
}

//	�¶ȱ���
static int CheckTemp(void)
{
	//	���±���
	if( hsio.temp < 10 ){
		stored_step = step;
		stored_state = state;
		state = 0;	//	������ͣ
		return 1;
	}
	//	���±���
	if( hsio.temp > hscfg.Temp*10 ){
		stored_step = step;
		stored_state = state;
		state = 1;	//	������ͣ
		return 1;
	}
	//	����
	return 0;
}

static void StateSTB(void)
{
	switch( state ){
		case	0:	//	�ȴ��ص�λ
			DevState("�ȴ��ص�λ", COLOR_ORANGE);
			//	�ر������豸
			hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);		//	�ر���ˮ����ת�淧
			hwio.out &= ~(HWIO_OBIT_YSB|HWIO_OBIT_POWER);	//	�ر���ˮ�ú͵���Դ
			break;
		case	1:	//	�ȴ�����
			DevState("�ȴ�����", COLOR_GREEN);
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	����״̬���ر������豸���ȴ�����
static int StepSTB(void)
{
	printf("%s()\r\n", __func__);

	//	״̬��ʼ��
	StateSTB();

	//	��ѭ��
	for(;;){

		//	��̨����
		HS_Idle();

		//	���½���
		Page2Update();

		//	��������
		Page2Input();

		//	�˳���ǰҳ��
		if( CheckExit() ) return STEP_EXIT;

		//	�¶ȱ���
		if( CheckTemp() ) return STEP_ERR;

		//	״̬������
		switch( state ){
			case	0:	//	�ȴ��ص�λ
				//	�ص�λ
				if( (hwio.in & (HWIO_IBIT_QSX|HWIO_IBIT_ZZX))==(HWIO_IBIT_QSX|HWIO_IBIT_ZZX)){
					state = 1;
					StateSTB();
				}
				break;
			case	1:	//	�ȴ�����
				//	����
				if( input & INPUT_START_STOP ){
					state = 0;
					return	STEP_YSB;
				}
				break;
		}
	}
}

static void StateYSB(void)
{
	switch( state ){
		case	0:	//	�ȴ�Һλ��
			DevState("�ȴ�Һλ��", COLOR_ORANGE);
			break;
		case	1:	//	�ȴ�Һλ��
			DevState("װ����ˮ��", COLOR_GREEN);
			//	����ˮ��
			hwio.out |= HWIO_OBIT_YSB;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	��ˮ�ã�Һλ�� >> ����ˮ�� >> ֱ��Һλ��
static int StepYSB(void)
{
	printf("%s()\r\n", __func__);

	//	״̬��ʼ��
	StateYSB();

	//	��ѭ��
	for(;;){

		//	��̨����
		HS_Idle();

		//	���½���
		Page2Update();

		//	��������
		Page2Input();

		//	�˳���ǰҳ��
		if( CheckExit() ) return STEP_EXIT;

		//	ֹͣ
		if( CheckStop() ) return STEP_STB;

		//	�¶ȱ���
		if( CheckTemp() ) return STEP_ERR;

		//	״̬������
		switch( state ){
			case	0:	//	�ȴ�Һλ��
				//	Һλ��
				if( hwio.in & HWIO_IBIT_YWD ){
					state = 1;
					StateYSB();
				}
				break;
			case	1:	//	�ȴ�Һλ��
				//	Һλ��
				if( hwio.in & HWIO_IBIT_YWZ ){
					state = 0;
					hwio.out &= ~HWIO_OBIT_YSB;
					return STEP_QSF;
				}
				break;
		}
	}
}

static void StateQSF(void)
{
	switch( state ){
		case	0:	//	�ȴ�Һλ��
			DevState("ȷ��Һλ��", COLOR_ORANGE);
			break;
		case	1:	//	�ȴ�Һλ��
			DevState("װ����ˮ��", COLOR_GREEN);
			//	����ˮ��
			hsio.out |= HSIO_OBIT_QS;
			break;
		case	2:	//	�ȴ��ص�λ
			DevState("�ȴ��ص�λ", COLOR_ORANGE);
			//	�ر���ˮ��
			hsio.out &= ~HSIO_OBIT_QS;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	��ˮ����Һλ�� >> ����ˮ�� >> ֱ��Һλ��
static int StepQSF(void)
{
	printf("%s()\r\n", __func__);

	//	״̬��ʼ��
	StateQSF();

	//	��ѭ��
	for(;;){

		//	��̨����
		HS_Idle();

		//	���½���
		Page2Update();

		//	��������
		Page2Input();

		//	�˳���ǰҳ��
		if( CheckExit() ) return STEP_EXIT;

		//	ֹͣ
		if( CheckStop() ) return STEP_STB;

		//	�¶ȱ���
		if( CheckTemp() ) return STEP_ERR;

		//	״̬������
		switch( state ){
			case	0:	//	�ȴ�Һλ��
				if( hwio.in & HWIO_IBIT_YWZ ){
					state = 1;
					StateQSF();
				}
				break;
			case	1:	//	�ȴ�Һλ��
				if( hwio.in & HWIO_IBIT_YWG ){
					state = 2;
					StateQSF();
				}
				break;
			case	2:	//	�ȴ��ص�λ
				if( hwio.in & HWIO_IBIT_QSX ){
					state = 0;
					return STEP_PWR;
				}
				break;
		}
	}
}

static void StatePWR(void)
{
	switch( state ){
		case	0:	//	�ر����нӿ�
			DevState("�ȴ��ص�λ",COLOR_ORANGE);
			//	�رշ��źͱ�
			hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);
			hwio.out &= ~HWIO_OBIT_YSB;
			break;
		case	1:	//	�����
			DevState("��������",COLOR_GREEN);
			//	�򿪵�Դ
			hwio.out |= HWIO_OBIT_POWER;
			//	��ʼ����ʱ
			if( time==0 ) time = hscfg.Time * 60;	//	��0��ʾ�����Ǵ�����״̬�ָ�������
			p2t.Updated = 0;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	��⣺QS�ص�λ&&Һλ�� >> �򿪵���Դ >> ֱ��ʱ�䵽
static int StepPWR(void)
{
	printf("%s()\r\n", __func__);

	//	״̬��ʼ��
	state = 0;
	StatePWR();

	//	��ѭ��
	for(;;){

		//	��̨����
		HS_Idle();

		//	���½���
		Page2Update();

		//	��������
		Page2Input();

		//	�˳���ǰҳ��
		if( CheckExit() ) return STEP_EXIT;

		//	ֹͣ
		if( CheckStop() ) return STEP_STB;

		//	�¶ȱ���
		if( CheckTemp() ) return STEP_ERR;

		//	��Դ����
		if( hwio.in & HWIO_IBIT_ERM ){
			stored_step = step;
			stored_state = state;
			state = 2;
			return STEP_ERR;
		}

		//	״̬������
		switch( state ){
			case	0:	//	�ȴ��ص�λ
				if( (hwio.in & (HWIO_IBIT_YWG|HWIO_IBIT_QSX|HWIO_IBIT_ZZX))==(HWIO_IBIT_YWG|HWIO_IBIT_QSX|HWIO_IBIT_ZZX) ){
					state = 1;
					StatePWR();
				}
				break;
			case	1:	//	�ȴ�ʱ�䵽
				if( p2t.Updated ){
					p2t.Updated = 0;
					if( time>0 ) time --;
					//	ʱ�䵽
					if( time==0 ){
						state = 0;
						hwio.out &= ~HWIO_OBIT_POWER;
						return STEP_ZZF;
					}
					//	����ʱ�䣺ÿ1���ӱ���һ��
					if( time%60==0 ){
						HS_SaveStepStateTime(step,state,time);
					}
				}
				break;
		}
	}
}

static void StateZZF(void)
{
	switch( state ){
		case	0:	//	��ҩ������
			DevState("��ҩ������",COLOR_RED);
			//	�ر�ת�淧
			hsio.out &= ~HSIO_OBIT_ZZ;
			break;
		case	1:	//	ת��ҩҺ��
			DevState("ת��ҩҺ��",COLOR_GREEN);
			//	��ת�淧
			hsio.out |= HSIO_OBIT_ZZ;
			break;
		case	2:	//	�ȴ��ص�λ
			DevState("�ȴ��ص�λ",COLOR_ORANGE);
			//	�ر�ת�淧
			hsio.out &= ~HSIO_OBIT_ZZ;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	ת�棺��ҩδ�� >> ��ת�淧 >> ֱ����ҩ��
static int StepZZF(void)
{
	printf("%s()\r\n", __func__);

	//	״̬��ʼ��
	StateZZF();

	//	��ѭ��
	for(;;){

		//	��̨����
		HS_Idle();

		//	���½���
		Page2Update();

		//	��������
		Page2Input();

		//	�˳���ǰҳ��
		if( CheckExit() ) return STEP_EXIT;

		//	ֹͣ
		if( CheckStop() ) return STEP_STB;

		//	�¶ȱ���
//		if( CheckTemp() ) return STEP_ERR;

		//	״̬������
		switch( state ){
			case	0:	//	�ȴ�δ��
				//	��ҩ������
				if( hwio.in & HWIO_IBIT_CYM ){
					if( p2t.Updated ){
						p2t.Updated = 0;
//						hwio.out ^= HWIO_OBIT_BEEPER;
					}
				}
				//	��ҩ��δ��
				else{
					hwio.out &= ~HWIO_OBIT_BEEPER;
					state = 1;
					StateZZF();
				}
				break;
			case	1:	//	ת����
				//	�ŵ�Һλ��
				if( hwio.in & HWIO_IBIT_YWD ){
					state = 2;
					StateZZF();
				}
//				//	��ҩ����
//				else if( hwio.in & HWIO_IBIT_CYM ){
//					state = 0;
//					StateZZF();
//				}
				break;
			case	2:	//	�ȴ��ر�
				if( hwio.in & HWIO_IBIT_ZZX ){
					state = 0;
					return STEP_ITV;
				}
				break;
		}
	}
}

static void StateERR(void)
{
	//	�ر�����
	hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);
	hwio.out &= ~(HWIO_OBIT_POWER|HWIO_OBIT_YSB);
	//	��ʾ״̬
	switch( state ){
		case	0:	//	�¶ȹ�����ͣ
			DevState("�¶ȹ�����ͣ",COLOR_RED);
			break;
		case	1:	//	�¶ȹ�����ͣ
			DevState("�¶ȹ�����ͣ",COLOR_RED);
			break;
		case	2:	//	��Դ������ͣ
			DevState("��Դ������ͣ",COLOR_RED);
			//	�򿪷�����
			hwio.out |= HWIO_OBIT_BEEPER;
			p2t.Updated = 0;
			p2t.Counter = 0;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	����
static int StepERR(void)
{
	printf("%s()\r\n", __func__);

	//	״̬��ʼ��
	StateERR();

	//	��ѭ��
	for(;;){

		//	��̨����
		HS_Idle();

		//	���½���
		Page2Update();

		//	��������
		Page2Input();

		//	�˳���ǰҳ��
		if( CheckExit() ) return STEP_EXIT;

		//	ֹͣ
		if( CheckStop() ) return STEP_STB;

		//	״̬������
		switch( state ){
			case	0:	//	����
				if( hsio.temp > 10 ){
					state = stored_state;
					return stored_step;
				}
				break;
			case	1:	//	����
				if( hsio.temp < hscfg.Temp*10 ){
					state = stored_state;
					return stored_step;
				}
				break;
			case	2:	//	��Դ����
				if( !(hwio.in & HWIO_IBIT_ERM) ){
					//	�رշ�����
					hwio.out &= ~HWIO_OBIT_BEEPER;
					state = stored_state;
					return stored_step;
				}else{
					//	����������
					if( p2t.Updated ){
						p2t.Updated = 0;
						hwio.out ^= HWIO_OBIT_BEEPER;
					}
				}
				break;
		}
	}
}

//	��Ъ����Ϣ5��
static int StepITV(void)
{
	printf("%s()\r\n", __func__);

	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);

	//	״̬��ʾ
	DevState("ѭ����Ъ��",COLOR_GREEN);

	//	�ر������豸
	hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);		//	�ر���ˮ����ת�淧
	hwio.out &= ~(HWIO_OBIT_YSB|HWIO_OBIT_POWER);	//	�ر���ˮ�ú͵���Դ

	//	��������ʱ
	time = 5;
	p2t.Updated = 0;

	//	��ѭ��
	for(;;){

		//	��̨����
		HS_Idle();

		//	���½���
		Page2Update();

		//	��������
		Page2Input();

		//	�˳���ǰҳ��
		if( CheckExit() ) return STEP_EXIT;

		//	ֹͣ
		if( CheckStop() ) return STEP_STB;

		//	�¶ȱ���
		if( CheckTemp() ) return STEP_ERR;

		//	���5�뵹��ʱ
		if( p2t.Updated ){
			p2t.Updated = 0;
			if( time>0 ) time --;
			//	��ʱ
			if( time==0 ) return STEP_YSB;
		}
	}
}

int HS_Page_2(void)
{
	int next;

	printf("%s()\r\n", __func__);

	//	��ʼ״̬
	if( hscfg.Mode & HSCFG_MODE_AUTO ){
		uint8_t a,b;
		uint32_t c;
		HS_LoadStepStateTime(&a,&b,&c);
		step = a;
		state = b;
		time = c;
		printf("Auto Mode: %d %d %d\r\n", step, state, time);
	}else{
//		step = STEP_STB;
//		state = 0;
//		time = 0;
	}

	next = step;

	//	ȷ�Ͻ��뻭��2
	HS_ConfirmScreen(2);

	//	ǿ�Ƹ��½���
	gs.step = STEP_EXIT;
	gs.out = gs.in = gs.hso = 0xffffffff;
	gs.time = gs.runtime = 0xffffffff;
	gs.curr = gs.volt = gs.temp = 0xffffffff;
	Page2Update();

	for(;;){

		//	���벻ͬ����
		switch( step ){
			case	STEP_STB:	next = StepSTB(); break;
			case	STEP_YSB:	next = StepYSB(); break;
			case	STEP_QSF:	next = StepQSF(); break;
			case	STEP_PWR:	next = StepPWR(); break;
			case	STEP_ZZF:	next = StepZZF(); break;
			case	STEP_ERR:	next = StepERR(); break;
			case	STEP_ITV:	next = StepITV(); break;
			default: next = state = 0; break;
		}
		
		//	���ػ��棱
		if( next==STEP_EXIT ) return 1;
	
		//	��һ��
		if( next!=step ){
			printf("step: %d -> %d\r\n", step, next);
			step = next;
		}
	}
}

