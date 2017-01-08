#include "bsp.h"
#include <string.h>
#include "ds18b20.h"

#define		SCL_H		GPIOB->BSRR = GPIO_Pin_8
#define		SCL_L		GPIOB->BRR  = GPIO_Pin_8
#define		SCL_read	GPIOB->IDR  & GPIO_Pin_8

#define		SDA_H		GPIOB->BSRR = GPIO_Pin_9
#define		SDA_L		GPIOB->BRR  = GPIO_Pin_9
#define		SDA_read	GPIOB->IDR  & GPIO_Pin_9

#define		I2C_delay()		delay_us(5)
#define		I2C_PageSize 	8  /* 24C02每页8字节 */

extern void delay_us(uint16_t us);

void delay_ms(uint16_t ms)
{
	while(ms>0){
		delay_us(1000);
		ms --;
	}
}

void BSP_I2CInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;	

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	//	PB8/9: OD
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//	set high
	SDA_H;
	SCL_H;

#if 0
	//	for delay
	DS_Init();

	delay_ms(1000);
	I2C_Test();

	while(1){
		delay_ms(1000);
		SDA_L;
		printf("SDA L\r\n");

		delay_ms(1000);
		SCL_L;
		printf("SCL L\r\n");

		delay_ms(1000);
		SDA_H;
		printf("SDA H\r\n");

		delay_ms(1000);
		SCL_H;
		printf("SCL H\r\n");
	}
#endif
}

static FunctionalState I2C_Start(void)
{
	SDA_H;
	SCL_H;
	I2C_delay();
	if(!SDA_read)return DISABLE;	/* SDA线为低电平则总线忙,退出 */
	SDA_L;
	I2C_delay();
	if(SDA_read) return DISABLE;	/* SDA线为高电平则总线出错,退出 */
	SDA_L;
	I2C_delay();
	return ENABLE;
}

static void I2C_Stop(void)
{
	SCL_L;
	I2C_delay();
	SDA_L;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SDA_H;
	I2C_delay();
}

static void I2C_Ack(void)
{	
	SCL_L;
	I2C_delay();
	SDA_L;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SCL_L;
	I2C_delay();
}

static void I2C_NoAck(void)
{	
	SCL_L;
	I2C_delay();
	SDA_H;
	I2C_delay();
	SCL_H;
	I2C_delay();
	SCL_L;
	I2C_delay();
}

static FunctionalState I2C_WaitAck(void) 	
{
	SCL_L;
	I2C_delay();
	SDA_H;			
	I2C_delay();
	SCL_H;
	I2C_delay();
	if(SDA_read)
	{
      SCL_L;
      return DISABLE;
	}
	SCL_L;
	return ENABLE;
}

static void I2C_SendByte(uint8_t SendByte) 
{
    uint8_t i=8;
    while(i--)
    {
        SCL_L;
        I2C_delay();
      if(SendByte&0x80)
        SDA_H;  
      else 
        SDA_L;   
        SendByte<<=1;
        I2C_delay();
		SCL_H;
        I2C_delay();
    }
    SCL_L;
}

static uint8_t I2C_ReceiveByte(void)  
{ 
    uint8_t i=8;
    uint8_t ReceiveByte=0;

    SDA_H;				
    while(i--)
    {
      ReceiveByte<<=1;      
      SCL_L;
      I2C_delay();
	  SCL_H;
      I2C_delay();	
      if(SDA_read)
      {
        ReceiveByte|=0x01;
      }
    }
    SCL_L;
    return ReceiveByte;
}

FunctionalState I2C_WriteByte(uint8_t SendByte, uint16_t WriteAddress, uint8_t DeviceAddress)
{		
	delay_ms(10);
    if(!I2C_Start())return DISABLE;
    I2C_SendByte(((WriteAddress & 0x0700) >>7) | DeviceAddress & 0xFFFE); /*设置高起始地址+器件地址 */
    if(!I2C_WaitAck()){I2C_Stop(); return DISABLE;}
    I2C_SendByte((uint8_t)(WriteAddress & 0x00FF));   /* 设置低起始地址 */      
    I2C_WaitAck();	
    I2C_SendByte(SendByte);
    I2C_WaitAck();   
    I2C_Stop(); 
    return ENABLE;
}									 

FunctionalState I2C_ReadByte(uint8_t* pBuffer,   uint16_t length,   uint16_t ReadAddress,  uint8_t DeviceAddress)
{
    if(!I2C_Start())return DISABLE;
    I2C_SendByte(((ReadAddress & 0x0700) >>7) | DeviceAddress & 0xFFFE); /* 设置高起始地址+器件地址 */ 
    if(!I2C_WaitAck()){I2C_Stop(); return DISABLE;}
    I2C_SendByte((uint8_t)(ReadAddress & 0x00FF));   /* 设置低起始地址 */      
    I2C_WaitAck();
    I2C_Start();
    I2C_SendByte(((ReadAddress & 0x0700) >>7) | DeviceAddress | 0x0001);
    I2C_WaitAck();
    while(length)
    {
      *pBuffer = I2C_ReceiveByte();
      if(length == 1)I2C_NoAck();
      else I2C_Ack(); 
      pBuffer++;
      length--;
    }
    I2C_Stop();
    return ENABLE;
}

FunctionalState EEP_Read(uint8_t* pBuffer, uint16_t length, uint16_t ReadAddress, uint8_t DeviceAddress)
{
//	printf("%s(%x,%x,%x,%x)\r\n", __func__, pBuffer[0], length, ReadAddress, DeviceAddress);

    if(!I2C_Start()) goto EEP_READ_ERROR;

    I2C_SendByte(((ReadAddress & 0x0700) >>7) | DeviceAddress & 0xFFFE); /* 设置高起始地址+器件地址 */ 
    if(!I2C_WaitAck()) goto EEP_READ_ERROR;

    I2C_SendByte((uint8_t)(ReadAddress & 0x00FF));   /* 设置低起始地址 */      
    if(!I2C_WaitAck()) goto EEP_READ_ERROR;

    if(!I2C_Start()) goto EEP_READ_ERROR;

    I2C_SendByte(((ReadAddress & 0x0700) >>7) | DeviceAddress | 0x0001);
    if(!I2C_WaitAck()) goto EEP_READ_ERROR;

    while(length)
    {
      *pBuffer = I2C_ReceiveByte();
      if(length == 1) I2C_NoAck(); else I2C_Ack(); 

      pBuffer++;
      length--;
    }

    I2C_Stop();
    return ENABLE;

EEP_READ_ERROR:
	I2C_Stop();
	return DISABLE;
}

FunctionalState EEP_WritePage(uint8_t* pBuffer, uint16_t length, uint16_t WriteAddress, uint8_t DeviceAddress)
{		
//	printf("\t%s(%x,%x,%x,%x)\r\n", __func__, pBuffer[0], length, WriteAddress, DeviceAddress);

    if(!I2C_Start()) goto EEP_WRITE_PAGE_ERROR;

    I2C_SendByte(((WriteAddress & 0x0700) >>7) | DeviceAddress & 0xFFFE); /*设置高起始地址+器件地址 */
    if(!I2C_WaitAck()) goto EEP_WRITE_PAGE_ERROR;

    I2C_SendByte((uint8_t)(WriteAddress & 0x00FF));   /* 设置低起始地址 */      
    if(!I2C_WaitAck()) goto EEP_WRITE_PAGE_ERROR;

	while(length)
	{
    	I2C_SendByte(*pBuffer);
	    if(!I2C_WaitAck()) goto EEP_WRITE_PAGE_ERROR;

		pBuffer++;
		length--;
	}

    I2C_Stop(); 
	delay_ms(10);	//	等待写完毕
    return ENABLE;

EEP_WRITE_PAGE_ERROR:
	I2C_Stop();
	return DISABLE;
}									 

FunctionalState EEP_Write(uint8_t* pBuffer, uint16_t length, uint16_t WriteAddress, uint8_t DeviceAddress)
{
//	printf("%s(%x,%x,%x,%x)\r\n", __func__, pBuffer[0], length, WriteAddress, DeviceAddress);

	//	第一页
	if( WriteAddress % I2C_PageSize != 0 ){
		uint8_t offset, btw;

		offset = WriteAddress % I2C_PageSize;
		btw = length;
		if( btw > I2C_PageSize-offset ) btw = I2C_PageSize-offset;

		if( EEP_WritePage(pBuffer,btw,WriteAddress,DeviceAddress)==DISABLE ) return DISABLE;

		pBuffer += btw;
		WriteAddress += btw;
		length -= btw;
	}

	//	中间页
	while( length>=I2C_PageSize ){
		if( EEP_WritePage(pBuffer,I2C_PageSize,WriteAddress,DeviceAddress)==DISABLE ) return DISABLE;

		pBuffer += I2C_PageSize;
		WriteAddress += I2C_PageSize;
		length -= I2C_PageSize;

	}

	//	最后一页
	if( length>0 ){
		if( EEP_WritePage(pBuffer,length,WriteAddress,DeviceAddress)==DISABLE ) return DISABLE;

		pBuffer += length;
		WriteAddress += length;
		length -= length;
	}

	return ENABLE;
}									 

void I2C_Test(void)
{
	uint16_t Addr;
	static uint8_t WriteBuffer[256],ReadBuffer[256];

	/* EEPROM读数据 */
	printf("\r\nEEPROM Read\r\n");
	EEP_Read(ReadBuffer, sizeof(WriteBuffer),0, ADDR_24C08);
	for(Addr=0; Addr<256; Addr++){
		if( Addr%32==0 ) printf("\r\n");
		printf("%3x",ReadBuffer[Addr]);
	}

	printf("\r\nEEPROM Write\r\n");
	for(Addr=0; Addr<256; Addr++)
	{
		WriteBuffer[Addr]=ReadBuffer[1]+Addr;	 /* 填充WriteBuffer */
		if( Addr%32==0 ) printf("\r\n");
		printf("%3x",WriteBuffer[Addr]);
    }
	/* 开始向EEPROM写数据 */
	for(Addr=0;Addr<16;Addr++){
		EEP_Write(&WriteBuffer[Addr*16], Addr, Addr*16, ADDR_24C08);
		EEP_Write(&WriteBuffer[Addr*16+Addr], 16-Addr, Addr*16+Addr, ADDR_24C08);
	}
}
