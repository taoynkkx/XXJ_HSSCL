#include "hsscl.h"
#include <stdio.h>
#include "gzdc.h"
#include "hsio.h"
#include "hwio.h"
#include <string.h>

#define		STEP_STB		0	//	待机
#define		STEP_YSB		1	//	盐水泵
#define		STEP_QSF		2	//	清水阀
#define		STEP_PWR		3	//	电解电源
#define		STEP_ZZF		4	//	转存阀
#define		STEP_ITV		5	//	循环间歇
#define		STEP_ERR		6	//	故障暂停
#define		STEP_EXIT		9	//	退出

#define		COLOR_GREEN		0x0600
#define		COLOR_RED		0xF800
#define		COLOR_ORANGE	0xFB20

static uint32_t step;	//	运行步骤
static uint32_t state;	//	子状态
static uint32_t time;	//	倒计时
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

//	检查状态，需要时更新
static void Page2Update(void)
{
	//	启停按钮　和　步骤指示
	if( gs.step!=step ){
		if( step==STEP_STB ) DC_TX_SetIcon(2,9,0); else DC_TX_SetIcon(2,9,1);
		DC_TX_SetIcon(2,11,step);
		//	保存状态
		gs.step = step;
	}
	//	输入信号
	if( gs.in!=hwio.in ){
		//	清水阀到位
		if( hwio.in & HWIO_IBIT_QSX ) DC_TX_SetVisible(2,14,0); else DC_TX_SetVisible(2,14,1);
		//	转存阀到位
		if( hwio.in & HWIO_IBIT_ZZX ) DC_TX_SetVisible(2,16,0); else DC_TX_SetVisible(2,16,1);
		//	液位指示
		if( hwio.in & HWIO_IBIT_YWG ) DC_TX_SetIcon(2,15,7);
		else if( hwio.in & HWIO_IBIT_YWZ ) DC_TX_SetIcon(2,15,2);
		else if( hwio.in & HWIO_IBIT_YWD ) DC_TX_SetIcon(2,15,0);
		else DC_TX_SetIcon(2,15,0xff);
		//	外部电源故障信号
		if( hwio.in & HWIO_IBIT_ERM ){
//			const char msg[] = "!*电源故障*!";
//			strcpy((void*)DC_TX_Buffer, (void*)msg);
//			DC_TX_SetText(2,17,strlen(msg));
//			DC_TX_SetFGColor(2,17,COLOR_RED);
		}
		//	保存状态
		gs.in = hwio.in;
	}
	//	输出信号
	if( gs.out!=hwio.out ){
		//	盐水泵
		if( hwio.out & HWIO_OBIT_YSB ) DC_TX_SetVisible(2,13,1); else DC_TX_SetVisible(2,13,0);
		//	电解电源
		if( hwio.out & HWIO_OBIT_POWER ) DC_TX_SetVisible(2,7,1); else DC_TX_SetVisible(2,7,0);
		//	保存状态
		gs.out = hwio.out;
	}
	//	倒计时
	if( gs.time!=time ){
		uint32_t m;
		m = time / 60;
		sprintf((void*)DC_TX_Buffer,"%d:%d",m/60,m%60);
		DC_TX_SetText(2,3,strlen((void*)DC_TX_Buffer));
		//	保存状态
		gs.time = time;
	}
	//	总时间(分)
	if( gs.runtime!=runtime ){
		sprintf((void*)DC_TX_Buffer,"%u",runtime/60);	//	小时
		DC_TX_SetText(2,5,strlen((void*)DC_TX_Buffer));
		//	保存状态
		gs.runtime = runtime;
	}
	//	电流
	if( gs.curr!=hsio.curr ){
		sprintf((void*)DC_TX_Buffer,"%d", hsio.curr);
		DC_TX_SetText(2,2,strlen((void*)DC_TX_Buffer));
		//	保存状态
		gs.curr = hsio.curr;
	}
	//	电压
	if( gs.volt!=hsio.volt ){
		sprintf((void*)DC_TX_Buffer,"%d", hsio.volt);
		DC_TX_SetText(2,1,strlen((void*)DC_TX_Buffer));
		//	保存状态
		gs.volt = hsio.volt;
	}
	//	温度
	if( gs.temp!=hsio.temp ){
		sprintf((void*)DC_TX_Buffer,"%d.%d", hsio.temp/10, hsio.temp%10);
		DC_TX_SetText(2,4,strlen((void*)DC_TX_Buffer));
		//	保存状态
		gs.temp = hsio.temp;
	}
}

//	显示设备状态
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

//	界面输入
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
		if( RX_CONTROL_ID==9 ){	//	启停
			if( RX_CONTROL_TYPE==1 ){	//	按下
				input |= INPUT_START_STOP;
			}
		}
		if( RX_CONTROL_ID==8 ){	//	返回
			if( RX_CONTROL_TYPE==1 ){	//	按下
				input |= INPUT_RETURN_DOWN;
			}else if( RX_CONTROL_TYPE==0 ){	//	弹起
				input |= INPUT_RETURN_UP;
			}
		}
	}
}

//	界面退出
static int CheckExit(void)
{
	//	退出
	if( input & INPUT_RETURN_UP ) return 1;
	//	关闭所有设备
	if( input & INPUT_RETURN_DOWN ){
		hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);		//	关闭清水阀和转存阀
		hwio.out &= ~(HWIO_OBIT_YSB|HWIO_OBIT_POWER);	//	关闭盐水泵和电解电源
	}
	//	没有退出
	return	0;
}

//	停止
static int CheckStop(void)
{
	if( input & INPUT_START_STOP ){
		state = 0;
		return 1;
	}
	return 0;
}

//	温度保护
static int CheckTemp(void)
{
	//	低温保护
	if( hsio.temp < 10 ){
		stored_step = step;
		stored_state = state;
		state = 0;	//	低温暂停
		return 1;
	}
	//	高温保护
	if( hsio.temp > hscfg.Temp*10 ){
		stored_step = step;
		stored_state = state;
		state = 1;	//	高温暂停
		return 1;
	}
	//	正常
	return 0;
}

static void StateSTB(void)
{
	switch( state ){
		case	0:	//	等待关到位
			DevState("等待关到位", COLOR_ORANGE);
			//	关闭所有设备
			hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);		//	关闭清水阀和转存阀
			hwio.out &= ~(HWIO_OBIT_YSB|HWIO_OBIT_POWER);	//	关闭盐水泵和电解电源
			break;
		case	1:	//	等待启动
			DevState("等待启动", COLOR_GREEN);
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	待机状态：关闭所有设备，等待启动
static int StepSTB(void)
{
	printf("%s()\r\n", __func__);

	//	状态初始化
	StateSTB();

	//	主循环
	for(;;){

		//	后台处理
		HS_Idle();

		//	更新界面
		Page2Update();

		//	界面输入
		Page2Input();

		//	退出当前页面
		if( CheckExit() ) return STEP_EXIT;

		//	温度保护
		if( CheckTemp() ) return STEP_ERR;

		//	状态机处理
		switch( state ){
			case	0:	//	等待关到位
				//	关到位
				if( (hwio.in & (HWIO_IBIT_QSX|HWIO_IBIT_ZZX))==(HWIO_IBIT_QSX|HWIO_IBIT_ZZX)){
					state = 1;
					StateSTB();
				}
				break;
			case	1:	//	等待启动
				//	启动
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
		case	0:	//	等待液位低
			DevState("等待液位低", COLOR_ORANGE);
			break;
		case	1:	//	等待液位中
			DevState("装入盐水中", COLOR_GREEN);
			//	打开盐水泵
			hwio.out |= HWIO_OBIT_YSB;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	盐水泵：液位低 >> 打开盐水泵 >> 直到液位中
static int StepYSB(void)
{
	printf("%s()\r\n", __func__);

	//	状态初始化
	StateYSB();

	//	主循环
	for(;;){

		//	后台处理
		HS_Idle();

		//	更新界面
		Page2Update();

		//	界面输入
		Page2Input();

		//	退出当前页面
		if( CheckExit() ) return STEP_EXIT;

		//	停止
		if( CheckStop() ) return STEP_STB;

		//	温度保护
		if( CheckTemp() ) return STEP_ERR;

		//	状态机处理
		switch( state ){
			case	0:	//	等待液位低
				//	液位低
				if( hwio.in & HWIO_IBIT_YWD ){
					state = 1;
					StateYSB();
				}
				break;
			case	1:	//	等待液位中
				//	液位中
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
		case	0:	//	等待液位中
			DevState("确认液位中", COLOR_ORANGE);
			break;
		case	1:	//	等待液位高
			DevState("装入清水中", COLOR_GREEN);
			//	打开清水阀
			hsio.out |= HSIO_OBIT_QS;
			break;
		case	2:	//	等待关到位
			DevState("等待关到位", COLOR_ORANGE);
			//	关闭清水阀
			hsio.out &= ~HSIO_OBIT_QS;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	清水阀：液位中 >> 打开清水阀 >> 直到液位高
static int StepQSF(void)
{
	printf("%s()\r\n", __func__);

	//	状态初始化
	StateQSF();

	//	主循环
	for(;;){

		//	后台处理
		HS_Idle();

		//	更新界面
		Page2Update();

		//	界面输入
		Page2Input();

		//	退出当前页面
		if( CheckExit() ) return STEP_EXIT;

		//	停止
		if( CheckStop() ) return STEP_STB;

		//	温度保护
		if( CheckTemp() ) return STEP_ERR;

		//	状态机处理
		switch( state ){
			case	0:	//	等待液位中
				if( hwio.in & HWIO_IBIT_YWZ ){
					state = 1;
					StateQSF();
				}
				break;
			case	1:	//	等待液位高
				if( hwio.in & HWIO_IBIT_YWG ){
					state = 2;
					StateQSF();
				}
				break;
			case	2:	//	等待关到位
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
		case	0:	//	关闭所有接口
			DevState("等待关到位",COLOR_ORANGE);
			//	关闭阀门和泵
			hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);
			hwio.out &= ~HWIO_OBIT_YSB;
			break;
		case	1:	//	电解中
			DevState("电解进行中",COLOR_GREEN);
			//	打开电源
			hwio.out |= HWIO_OBIT_POWER;
			//	初始化计时
			if( time==0 ) time = hscfg.Time * 60;	//	非0表示可能是从其他状态恢复过来的
			p2t.Updated = 0;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	电解：QS关到位&&液位高 >> 打开电解电源 >> 直到时间到
static int StepPWR(void)
{
	printf("%s()\r\n", __func__);

	//	状态初始化
	state = 0;
	StatePWR();

	//	主循环
	for(;;){

		//	后台处理
		HS_Idle();

		//	更新界面
		Page2Update();

		//	界面输入
		Page2Input();

		//	退出当前页面
		if( CheckExit() ) return STEP_EXIT;

		//	停止
		if( CheckStop() ) return STEP_STB;

		//	温度保护
		if( CheckTemp() ) return STEP_ERR;

		//	电源故障
		if( hwio.in & HWIO_IBIT_ERM ){
			stored_step = step;
			stored_state = state;
			state = 2;
			return STEP_ERR;
		}

		//	状态机处理
		switch( state ){
			case	0:	//	等待关到位
				if( (hwio.in & (HWIO_IBIT_YWG|HWIO_IBIT_QSX|HWIO_IBIT_ZZX))==(HWIO_IBIT_YWG|HWIO_IBIT_QSX|HWIO_IBIT_ZZX) ){
					state = 1;
					StatePWR();
				}
				break;
			case	1:	//	等待时间到
				if( p2t.Updated ){
					p2t.Updated = 0;
					if( time>0 ) time --;
					//	时间到
					if( time==0 ){
						state = 0;
						hwio.out &= ~HWIO_OBIT_POWER;
						return STEP_ZZF;
					}
					//	保存时间：每1分钟保存一次
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
		case	0:	//	储药箱已满
			DevState("储药箱已满",COLOR_RED);
			//	关闭转存阀
			hsio.out &= ~HSIO_OBIT_ZZ;
			break;
		case	1:	//	转存药液中
			DevState("转存药液中",COLOR_GREEN);
			//	打开转存阀
			hsio.out |= HSIO_OBIT_ZZ;
			break;
		case	2:	//	等待关到位
			DevState("等待关到位",COLOR_ORANGE);
			//	关闭转存阀
			hsio.out &= ~HSIO_OBIT_ZZ;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	转存：储药未满 >> 打开转存阀 >> 直到储药满
static int StepZZF(void)
{
	printf("%s()\r\n", __func__);

	//	状态初始化
	StateZZF();

	//	主循环
	for(;;){

		//	后台处理
		HS_Idle();

		//	更新界面
		Page2Update();

		//	界面输入
		Page2Input();

		//	退出当前页面
		if( CheckExit() ) return STEP_EXIT;

		//	停止
		if( CheckStop() ) return STEP_STB;

		//	温度保护
//		if( CheckTemp() ) return STEP_ERR;

		//	状态机处理
		switch( state ){
			case	0:	//	等待未满
				//	储药箱已满
				if( hwio.in & HWIO_IBIT_CYM ){
					if( p2t.Updated ){
						p2t.Updated = 0;
//						hwio.out ^= HWIO_OBIT_BEEPER;
					}
				}
				//	储药箱未满
				else{
					hwio.out &= ~HWIO_OBIT_BEEPER;
					state = 1;
					StateZZF();
				}
				break;
			case	1:	//	转存中
				//	放到液位低
				if( hwio.in & HWIO_IBIT_YWD ){
					state = 2;
					StateZZF();
				}
//				//	储药箱满
//				else if( hwio.in & HWIO_IBIT_CYM ){
//					state = 0;
//					StateZZF();
//				}
				break;
			case	2:	//	等待关闭
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
	//	关闭所有
	hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);
	hwio.out &= ~(HWIO_OBIT_POWER|HWIO_OBIT_YSB);
	//	显示状态
	switch( state ){
		case	0:	//	温度过低暂停
			DevState("温度过低暂停",COLOR_RED);
			break;
		case	1:	//	温度过高暂停
			DevState("温度过高暂停",COLOR_RED);
			break;
		case	2:	//	电源故障暂停
			DevState("电源故障暂停",COLOR_RED);
			//	打开蜂鸣器
			hwio.out |= HWIO_OBIT_BEEPER;
			p2t.Updated = 0;
			p2t.Counter = 0;
			break;
	}
	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);
}

//	故障
static int StepERR(void)
{
	printf("%s()\r\n", __func__);

	//	状态初始化
	StateERR();

	//	主循环
	for(;;){

		//	后台处理
		HS_Idle();

		//	更新界面
		Page2Update();

		//	界面输入
		Page2Input();

		//	退出当前页面
		if( CheckExit() ) return STEP_EXIT;

		//	停止
		if( CheckStop() ) return STEP_STB;

		//	状态机处理
		switch( state ){
			case	0:	//	低温
				if( hsio.temp > 10 ){
					state = stored_state;
					return stored_step;
				}
				break;
			case	1:	//	高温
				if( hsio.temp < hscfg.Temp*10 ){
					state = stored_state;
					return stored_step;
				}
				break;
			case	2:	//	电源故障
				if( !(hwio.in & HWIO_IBIT_ERM) ){
					//	关闭蜂鸣器
					hwio.out &= ~HWIO_OBIT_BEEPER;
					state = stored_state;
					return stored_step;
				}else{
					//	蜂鸣器鸣叫
					if( p2t.Updated ){
						p2t.Updated = 0;
						hwio.out ^= HWIO_OBIT_BEEPER;
					}
				}
				break;
		}
	}
}

//	间歇：休息5秒
static int StepITV(void)
{
	printf("%s()\r\n", __func__);

	if( hscfg.Mode & HSCFG_MODE_AUTO ) HS_SaveStepStateTime(step,state,time);

	//	状态提示
	DevState("循环间歇中",COLOR_GREEN);

	//	关闭所有设备
	hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);		//	关闭清水阀和转存阀
	hwio.out &= ~(HWIO_OBIT_YSB|HWIO_OBIT_POWER);	//	关闭盐水泵和电解电源

	//	启动倒计时
	time = 5;
	p2t.Updated = 0;

	//	主循环
	for(;;){

		//	后台处理
		HS_Idle();

		//	更新界面
		Page2Update();

		//	界面输入
		Page2Input();

		//	退出当前页面
		if( CheckExit() ) return STEP_EXIT;

		//	停止
		if( CheckStop() ) return STEP_STB;

		//	温度保护
		if( CheckTemp() ) return STEP_ERR;

		//	检测5秒倒计时
		if( p2t.Updated ){
			p2t.Updated = 0;
			if( time>0 ) time --;
			//	到时
			if( time==0 ) return STEP_YSB;
		}
	}
}

int HS_Page_2(void)
{
	int next;

	printf("%s()\r\n", __func__);

	//	初始状态
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

	//	确认进入画面2
	HS_ConfirmScreen(2);

	//	强制更新界面
	gs.step = STEP_EXIT;
	gs.out = gs.in = gs.hso = 0xffffffff;
	gs.time = gs.runtime = 0xffffffff;
	gs.curr = gs.volt = gs.temp = 0xffffffff;
	Page2Update();

	for(;;){

		//	进入不同步骤
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
		
		//	返回画面１
		if( next==STEP_EXIT ) return 1;
	
		//	下一步
		if( next!=step ){
			printf("step: %d -> %d\r\n", step, next);
			step = next;
		}
	}
}

