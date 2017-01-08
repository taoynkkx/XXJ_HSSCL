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
	// �½ڵ㲻��Ϊ��
	if( t==0 ) return;
	// ���ݳ�ʼ��
	t->Counter = t->StartTime;
	t->Updated = 0;
	t->Next = 0;
	// ��һ���ڵ�
	if( Head==0 ){
		Head = t;
		Tail = t;
		return;
	}
	//	��ӵ�β��
	Tail->Next = t;
	//	�Լ���Ϊ�µ�β��
	Tail = t;
}

void Timer_Task(void)
{
	Timer_t * t;
	// ����ʱ�仹û��
	if( Trace==Base ) return;
	// ���¸��������
	Trace = Base;
	// ����ÿһ����ʱ��
	t = Head;
	while( t ){
		//  ���ڷ���
		if( t->CycleTime > 0 ){
			//	��ʱ����
			t->Counter ++;
			//	��ʱ�ж�
			if( t->Counter > t->CycleTime ){
				t->Counter -= t->CycleTime;
				//	����֪ͨ
				t->Updated ++;
			}
		}
		//	��һ����ʱ��
		t = t->Next;		
	}
}

