#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>

typedef struct
{
  // ����
  uint32_t StartTime;
  uint32_t CycleTime;
  // ����
  uint32_t Counter;
  // ���
  uint32_t Updated;
  // ����
  void* Next;
}
Timer_t;

extern void Timer_Init(void);
extern void Timer_Task(void);
extern void Timer_TickUp(void);
extern void Timer_Load(Timer_t*);

#endif // __TIMER_H

