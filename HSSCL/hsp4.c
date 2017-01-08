#include "hsscl.h"
#include <stdio.h>
#include "gzdc.h"
#include <string.h>

#define	PASSWORD	"63207633"
#define	LENGTH		8
static uint8_t password[LENGTH];

int HS_Page_4(void)
{
	printf("%s()\r\n", __func__);

	//	ȷ�Ͻ��뻭��4
	HS_ConfirmScreen(4);

	//	ȷ��ֹͣ������ʾ
	DC_TX_StopGif(4,4);

	//	������뻺��
	memset(password,0,LENGTH);

	for(;;){
		int r;

		HS_Idle();

		//	��ѯ��������
		r = DC_RX_FindCmd();
		if( r>0 
			&& RX_CMD_TYPE==0xB1 
			&& RX_CTRL_MSG==0x11 
			&& RX_SCREEN_ID==4 ){
			//	���ذ�ť�����𣬷��ػ���1
			if( RX_CONTROL_ID==1 && RX_CONTROL_TYPE==0x10 ) return 1;
			//	������������
			if( RX_CONTROL_ID==3 && RX_CONTROL_TYPE==0x11 && r==21) memcpy(password, &RX_DATA, LENGTH);
			//	ȷ�ϰ����������ж�����
			if( RX_CONTROL_ID==2 && RX_CONTROL_TYPE==0x10 ){
				//	������ȷ
				if( memcmp(password, PASSWORD, LENGTH)==0 ) return 5; // �����������
				//	�������
				DC_TX_PlayGif(4,4);
			}
		}

	}
}

