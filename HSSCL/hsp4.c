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

	//	确认进入画面4
	HS_ConfirmScreen(4);

	//	确认停止错误提示
	DC_TX_StopGif(4,4);

	//	清除密码缓存
	memset(password,0,LENGTH);

	for(;;){
		int r;

		HS_Idle();

		//	查询界面输入
		r = DC_RX_FindCmd();
		if( r>0 
			&& RX_CMD_TYPE==0xB1 
			&& RX_CTRL_MSG==0x11 
			&& RX_SCREEN_ID==4 ){
			//	返回按钮，弹起，返回画面1
			if( RX_CONTROL_ID==1 && RX_CONTROL_TYPE==0x10 ) return 1;
			//	保存输入密码
			if( RX_CONTROL_ID==3 && RX_CONTROL_TYPE==0x11 && r==21) memcpy(password, &RX_DATA, LENGTH);
			//	确认按键，弹起，判断密码
			if( RX_CONTROL_ID==2 && RX_CONTROL_TYPE==0x10 ){
				//	密码正确
				if( memcmp(password, PASSWORD, LENGTH)==0 ) return 5; // 进入参数界面
				//	密码错误
				DC_TX_PlayGif(4,4);
			}
		}

	}
}

