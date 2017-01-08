#ifndef  __RS485_H
#define  __RS485_H

#include <stdint.h>

extern void RS485_Init(void);
extern void RS485_Task(void);

extern void RS485_Putc(uint8_t c);
extern void RS485_Putb(uint8_t * buf, int len);
extern void RS485_Puts(uint8_t * str);

#endif  //  __RS485_H

