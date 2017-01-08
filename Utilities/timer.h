#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>

typedef struct
{
  // 设置
  uint32_t StartTime;
  uint32_t CycleTime;
  // 运行
  uint32_t Counter;
  // 输出
  uint32_t Updated;
  // 链表
  void* Next;
}
Timer_t;

extern void Timer_Init(void);
extern void Timer_Task(void);
extern void Timer_TickUp(void);
extern void Timer_Load(Timer_t*);

#endif // __TIMER_H

