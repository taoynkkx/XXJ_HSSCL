#ifndef  __BSP_H
#define  __BSP_H

#include "stm32f10x.h"
#include <stdio.h>

//  统一初始化
extern void BSP_Init(void);

//	led
#define		LED_RCC_XXXX				RCC_APB2Periph_GPIOD
#define		LED_GPIO_PIN_XXXX			(GPIO_Pin_14|GPIO_Pin_15)
#define		LED_GPIOX					GPIOD
#define		LED_ON()					LED_GPIOX->BRR = LED_GPIO_PIN_XXXX
#define		LED_OFF()					LED_GPIOX->BSRR = LED_GPIO_PIN_XXXX
#define		LED_INV()					LED_GPIOX->ODR ^= LED_GPIO_PIN_XXXX
#define		LED1_ON()					LED_GPIOX->BRR  = GPIO_Pin_14
#define		LED1_OFF()					LED_GPIOX->BSRR = GPIO_Pin_14
#define		LED1_INV()					LED_GPIOX->ODR ^= GPIO_Pin_14
#define		LED2_ON()					LED_GPIOX->BRR  = GPIO_Pin_15
#define		LED2_OFF()					LED_GPIOX->BSRR = GPIO_Pin_15
#define		LED2_INV()					LED_GPIOX->ODR ^= GPIO_Pin_15
extern void BSP_LedInit(void);

//	tick
extern uint32_t Tick;
extern void BSP_TickInit(void);

//	gpio
extern void BSP_GPIOInit(void);

//	uart
extern void BSP_DebugInit(void);
extern int  BSP_DebugTxBusy(void);
extern void BSP_DebugSendData(int);

extern void BSP_RS485Init(void);
extern int  BSP_RS485TxBusy(void);
extern void BSP_RS485SendData(int);
extern void BSP_RS485DirTx(void);
extern void BSP_RS485DirRx(void);

//	can
extern void BSP_CanInit(void);
extern int BSP_CanSend(CanTxMsg*);

//	adc
extern uint16_t BSP_ADCRes[2];
extern void BSP_ADCInit(void);

//	i2c
extern void BSP_I2CInit(void);
extern FunctionalState I2C_WriteByte(uint8_t SendByte, uint16_t WriteAddress, uint8_t DeviceAddress);
extern FunctionalState I2C_ReadByte(uint8_t* pBuffer,   uint16_t length,   uint16_t ReadAddress,  uint8_t DeviceAddress);
extern void I2C_Test(void);

//	eeprom
#define		ADDR_24C08		0xA0
FunctionalState EEP_Read(uint8_t* pBuffer, uint16_t length, uint16_t ReadAddress, uint8_t DeviceAddress);
FunctionalState EEP_Write(uint8_t* pBuffer, uint16_t length, uint16_t WriteAddress, uint8_t DeviceAddress);

#endif  //  __BSP_H

