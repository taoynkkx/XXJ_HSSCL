#ifndef  __HWIO_H
#define  __HWIO_H

#include <stdint.h>

#define		HWIO_OBIT_BEEPER	0x1000
#define		HWIO_OBIT_YSB		0x0200
#define		HWIO_OBIT_POWER		0x0100
#define		HWIO_OBIT_QS4		0x0080
#define		HWIO_OBIT_QS3		0x0040
#define		HWIO_OBIT_QS2		0x0020
#define		HWIO_OBIT_QS1		0x0010
#define		HWIO_OBIT_ZZ4		0x0008
#define		HWIO_OBIT_ZZ3		0x0004
#define		HWIO_OBIT_ZZ2		0x0002
#define		HWIO_OBIT_ZZ1		0x0001

#define		HWIO_IBIT_ERM		(1<<5)
#define		HWIO_IBIT_CYM		(1<<6)
#define		HWIO_IBIT_YWD		(1<<7)
#define		HWIO_IBIT_YWZ		(1<<8)
#define		HWIO_IBIT_YWG		(1<<9)
#define		HWIO_IBIT_ZZO		(1<<10)
#define		HWIO_IBIT_ZZX		(1<<11)
#define		HWIO_IBIT_QSO		(1<<12)
#define		HWIO_IBIT_QSX		(1<<13)

typedef struct
{
	uint32_t in, out;
	uint16_t adc[2];
}
HWIO_t;

extern HWIO_t hwio;

extern void HWIO_Init(void);
extern void HWIO_Task(void);

#endif // __HWIO_H

