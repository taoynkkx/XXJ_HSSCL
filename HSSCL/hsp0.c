#include "hsscl.h"
#include <stdio.h>
#include "timer.h"
#include "gzdc.h"

int HS_Page_0(void)
{
	printf("%s()\r\n", __func__);

	//	确认进入画面0
	HS_ConfirmScreen(0);

	//	等待6秒
	HS_DelaySec(6);

	//	自动模式?
	if( hscfg.Mode & HSCFG_MODE_AUTO ) return 2;

	//	进入画面1
	return 1;

}

