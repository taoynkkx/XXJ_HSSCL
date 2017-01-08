#include "hsscl.h"
#include <stdio.h>
#include "gzdc.h"
#include "hwio.h"
#include "hsio.h"

static struct
{
	uint32_t in, out, hso;
}
gs;

static void Page3Update(void)
{
	//	�����ź�
	if( gs.in != hwio.in ){
		//	Һλָʾ�����е�
		if( hwio.in & HWIO_IBIT_YWG )    	DC_TX_SetIcon(3,14,7);
		else if( hwio.in & HWIO_IBIT_YWZ )	DC_TX_SetIcon(3,14,2);
		else if( hwio.in & HWIO_IBIT_YWD )	DC_TX_SetIcon(3,14,0);
		else DC_TX_SetIcon(3,14,0xff);
		//	Һλ������ʾ
		if( hwio.in & HWIO_IBIT_YWG ) DC_TX_SetVisible(3,15,0);
		//	��ˮ����λ
		if( hwio.in & HWIO_IBIT_QSX ) DC_TX_SetVisible(3,12,0); else DC_TX_SetVisible(3,12,1);
		//	ת�淧��λ
		if( hwio.in & HWIO_IBIT_ZZX ) DC_TX_SetVisible(3,13,0); else DC_TX_SetVisible(3,13,1);
		//	����״̬
		gs.in = hwio.in;
	}

	//	����ź�
	if( gs.out!=hwio.out ){
		int i = 0;
		//	��ˮ�ð�ť
		i = DC_TX_Put16(6, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hwio.out&HWIO_OBIT_YSB)?1:0, i);
		i = DC_TX_Put16(7, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hwio.out&HWIO_OBIT_YSB)?0:1, i);
		//	����Դ��ť
		i = DC_TX_Put16(8, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hwio.out&HWIO_OBIT_POWER)?1:0, i);
		i = DC_TX_Put16(9, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hwio.out&HWIO_OBIT_POWER)?0:1, i);
		//	��������
		DC_TX_MultiSet(3,i);
		//	��ˮ�ö���
		if( hwio.out & HWIO_OBIT_YSB ) DC_TX_SetVisible(3,11,1); else DC_TX_SetVisible(3,11,0);
		//	����Դ����
		if( hwio.out & HWIO_OBIT_POWER ) DC_TX_SetVisible(3,4,1); else DC_TX_SetVisible(3,4,0);
		//	����״̬
		gs.out = hwio.out;
	}

	//	����ź�
	if( gs.hso!=hsio.out ){
		int i = 0;
		//	��ˮ����ť
		i = DC_TX_Put16(1, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hsio.out&HSIO_OBIT_QS)?1:0, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hsio.out&HSIO_OBIT_QS)?0:1, i);
		//	ת�淧��ť
		i = DC_TX_Put16(3, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hsio.out&HSIO_OBIT_ZZ)?1:0, i);
		i = DC_TX_Put16(5, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hsio.out&HSIO_OBIT_ZZ)?0:1, i);
		//	��������
		DC_TX_MultiSet(3,i);
		//	����״̬
		gs.hso = hsio.out;
	}

}

static int Page3Input(void)
{
	switch( RX_CONTROL_ID ){
		case 10:	//	����
			if( RX_CONTROL_TYPE==1 ){		//	����
				DC_TX_SetIcon(3,10,1);		//	��ʾ����״̬
				hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);		//	�ر���ˮ����ת�淧
				hwio.out &= ~(HWIO_OBIT_YSB|HWIO_OBIT_POWER);	//	�ر���ˮ�ú͵���Դ
			}else if( RX_CONTROL_TYPE==0 ){	//	����
				DC_TX_SetIcon(3,10,0);		//	��ʾ����״̬
				return 1;					//	���ػ���1
			}
			break;
		case 6:		//	��ˮ�ô�
			if( RX_CONTROL_TYPE==1 ) hwio.out |= HWIO_OBIT_YSB;
			break;
		case 7:		//	��ˮ�ùر�
			if( RX_CONTROL_TYPE==1 ) hwio.out &= ~HWIO_OBIT_YSB;
			break;
		case	8:	//	����Դ��
			if( RX_CONTROL_TYPE==1 ){
				hwio.out |= HWIO_OBIT_POWER;
				if( !(hwio.in & HWIO_IBIT_YWG) ) DC_TX_SetVisible(3,15,1);	//	Һλ������ʾ
			}
			break;
		case	9:	//	����Դ�ر�
			if( RX_CONTROL_TYPE==1 ) hwio.out &= ~HWIO_OBIT_POWER;
			break;
		case	1:	//	��ˮ����
			if( RX_CONTROL_TYPE==1 ) hsio.out |= HSIO_OBIT_QS;
			break;
		case	2:	//	��ˮ���ر�
			if( RX_CONTROL_TYPE==1 ) hsio.out &= ~HSIO_OBIT_QS;
			break;
		case	3:	//	ת�淧��
			if( RX_CONTROL_TYPE==1 ) hsio.out |= HSIO_OBIT_ZZ;
			break;
		case	5:	//	ת�淧�ر�
			if( RX_CONTROL_TYPE==1 ) hsio.out &= ~HSIO_OBIT_ZZ;
			break;
	}

	//	���˳���ǰ����
	return -1;
}

int HS_Page_3(void)
{
	printf("%s()\r\n", __func__);

	//	ȷ�Ͻ��뻭��3
	HS_ConfirmScreen(3);

	//	ǿ�Ƹ���
	gs.in = gs.out = gs.hso = 0xffffffff;
	Page3Update();

	//	��ѭ��
	for(;;){
		int r;

		HS_Idle();

		//	���½���
		Page3Update();

		//	��ѯ��������
		r = DC_RX_FindCmd();
		if( r>0 && RX_CMD_TYPE==0xB1 && RX_CTRL_MSG==0x26 && RX_SCREEN_ID==3 ){
			r = Page3Input();
			if( r>=0 ) return r;	//	��������ҳ��
		}

	}
}

