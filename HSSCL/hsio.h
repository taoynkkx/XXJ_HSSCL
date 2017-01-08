#ifndef  __HSIO_H
#define  __HSIO_H

#include <stdint.h>

#define		HSIO_OBIT_QS	0x01
#define		HSIO_OBIT_ZZ	0x02

typedef struct
{
	uint32_t out;
	uint32_t volt;
	uint32_t curr;
	int32_t temp;
}
HSIO_t;

extern HSIO_t hsio;

extern void HSIO_Init(void);
extern void HSIO_Task(void);

#endif  //  __HSIO_H

