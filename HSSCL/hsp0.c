#include "hsscl.h"
#include <stdio.h>
#include "timer.h"
#include "gzdc.h"

int HS_Page_0(void)
{
	printf("%s()\r\n", __func__);

	//	ȷ�Ͻ��뻭��0
	HS_ConfirmScreen(0);

	//	�ȴ�6��
	HS_DelaySec(6);

	//	�Զ�ģʽ?
	if( hscfg.Mode & HSCFG_MODE_AUTO ) return 2;

	//	���뻭��1
	return 1;

}

