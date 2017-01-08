#include "hsscl.h"
#include <stdio.h>
#include "gzdc.h"
#include <string.h>
#include <stdlib.h>

static char buf[16];

static void Page5Update(void)
{
	int i = 0;
	int len;
	//	volt
	i = DC_TX_Put16(2,i);		//	control id
	sprintf(buf, "%d", hscfg.Volt); len = strlen(buf);
	i = DC_TX_Put16(len,i);		//	length
	i = DC_TX_PutBuf(buf,len,i);	//	data
	//	curr
	i = DC_TX_Put16(3,i);		//	control id
	sprintf(buf, "%d", hscfg.Curr); len = strlen(buf);
	i = DC_TX_Put16(len,i);		//	length
	i = DC_TX_PutBuf(buf,len,i);	//	data
	//	temp
	i = DC_TX_Put16(4,i);		//	control id
	sprintf(buf, "%d", hscfg.Temp); len = strlen(buf);
	i = DC_TX_Put16(len,i);		//	length
	i = DC_TX_PutBuf(buf,len,i);	//	data
	//	time
	i = DC_TX_Put16(5,i);		//	control id
	sprintf(buf, "%d", hscfg.Time); len = strlen(buf);
	i = DC_TX_Put16(len,i);		//	length
	i = DC_TX_PutBuf(buf,len,i);	//	data
	//	send
	DC_TX_MultiSet(5,i);
	//	auto mode
	if( hscfg.Mode & HSCFG_MODE_AUTO ){
		DC_TX_SetButton(5,1,0);
		DC_TX_SetButton(5,7,1);
	}else{
		DC_TX_SetButton(5,1,1);
		DC_TX_SetButton(5,7,0);
	}


}

int HS_Page_5(void)
{
	printf("%s()\r\n", __func__);

	//	确认进入画面5
	HS_ConfirmScreen(5);

	//	更新数据显示
	Page5Update();

	for(;;){
		int r;
		char * pEnd;

		HS_Idle();

		//	查询界面输入
		r = DC_RX_FindCmd();
		if( r>0
			&& RX_CMD_TYPE==0xB1 
			&& RX_CTRL_MSG==0x11 
			&& RX_SCREEN_ID==5 ){
			//	返回按钮，弹起，保存数据，返回画面1
			if( RX_CONTROL_ID==6 && RX_CONTROL_TYPE==0x10 ){
				HS_SaveConfig();
				return 1;
			}
			//	电压设置
			if( RX_CONTROL_ID==2 && RX_CONTROL_TYPE==0x11 ){
				uint32_t l = strtol((void*)&RX_DATA, &pEnd, 10);
				if( l>20 ) l = 20;
				if( l!=hscfg.Volt ){
					hscfg.Volt = l;
					Page5Update();
				}
			}
			//	电流设置
			if( RX_CONTROL_ID==3 && RX_CONTROL_TYPE==0x11 ){
				uint32_t l = strtol((void*)&RX_DATA, &pEnd, 10);
				if( l>300 ) l = 300;
				if( l!=hscfg.Curr ){
					hscfg.Curr = l;
					Page5Update();
				}
			}
			//	温度设置
			if( RX_CONTROL_ID==4 && RX_CONTROL_TYPE==0x11 ){
				uint32_t l = strtol((void*)&RX_DATA, &pEnd, 10);
				if( l>100 ) l = 100;
				if( l!=hscfg.Temp ){
					hscfg.Temp = l;
					Page5Update();
				}
			}
			//	时间设置
			if( RX_CONTROL_ID==5 && RX_CONTROL_TYPE==0x11 ){
				uint32_t l = strtol((void*)&RX_DATA, &pEnd, 10);
				if( l>999 ) l = 999;
				if( l!=hscfg.Time ){
					hscfg.Time = l;
					Page5Update();
				}
			}
			//	有人值守
			if( RX_CONTROL_ID==1 && RX_CONTROL_TYPE==0x10 ){
				if( hscfg.Mode & HSCFG_MODE_AUTO ){
					hscfg.Mode &= ~HSCFG_MODE_AUTO;
					Page5Update();
				}
			}
			//	无人值守
			if( RX_CONTROL_ID==7 && RX_CONTROL_TYPE==0x10 ){
				if( !(hscfg.Mode & HSCFG_MODE_AUTO) ){
					hscfg.Mode |= HSCFG_MODE_AUTO;
					Page5Update();
					HS_SaveStepStateTime(0,0,0);
				}
			}
		}
	}

}

