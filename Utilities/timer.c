#include "timer.h"

static Timer_t * Head = 0;
static Timer_t * Tail = 0;
static uint32_t Base = 0;
static uint32_t Trace = 0;

void Timer_Init(void)
{
	Head = Tail = 0;
	Trace = Base = 0;
}

void Timer_TickUp(void)
{
	Base ++;
}

void Timer_Load(Timer_t* t)
{
	// 新节点不能为空
	if( t==0 ) return;
	// 数据初始化
	t->Counter = t->StartTime;
	t->Updated = 0;
	t->Next = 0;
	// 第一个节点
	if( Head==0 ){
		Head = t;
		Tail = t;
		return;
	}
	//	添加到尾部
	Tail->Next = t;
	//	自己成为新的尾部
	Tail = t;
}

void Timer_Task(void)
{
	Timer_t * t;
	// 基本时间还没到
	if( Trace==Base ) return;
	// 更新跟随计数器
	Trace = Base;
	// 遍历每一个定时器
	t = Head;
	while( t ){
		//  周期非零
		if( t->CycleTime > 0 ){
			//	计时计数
			t->Counter ++;
			//	到时判断
			if( t->Counter > t->CycleTime ){
				t->Counter -= t->CycleTime;
				//	更新通知
				t->Updated ++;
			}
		}
		//	下一个定时器
		t = t->Next;		
	}
}

