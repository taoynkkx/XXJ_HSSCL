#include "hsscl.h"
#include <stdio.h>
#include "gzdc.h"

int HS_Page_1(void)
{
	printf("%s()\r\n", __func__);

	//	ȷ�Ͻ��뻭��1
	HS_ConfirmScreen(1);

	//	��鰴������
	for(;;){
		int r;

		HS_Idle();

		//	��ѯ��������
		r = DC_RX_FindCmd();
		if( r>0
			&& RX_CMD_TYPE==0xB1 
			&& RX_CTRL_MSG==0x11 
			&& RX_SCREEN_ID==0x01 
			&& RX_CONTROL_TYPE==0x10 )
		{
			if( RX_CONTROL_ID==0x01 ) return 2;	//	�Զ�����
			if( RX_CONTROL_ID==0x03 ) return 3;	//	�ֶ�����
			if( RX_CONTROL_ID==0x04 ) return 4;	//	��������
		}

	}

}

