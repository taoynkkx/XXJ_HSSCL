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
	//	输入信号
	if( gs.in != hwio.in ){
		//	液位指示：高中低
		if( hwio.in & HWIO_IBIT_YWG )    	DC_TX_SetIcon(3,14,7);
		else if( hwio.in & HWIO_IBIT_YWZ )	DC_TX_SetIcon(3,14,2);
		else if( hwio.in & HWIO_IBIT_YWD )	DC_TX_SetIcon(3,14,0);
		else DC_TX_SetIcon(3,14,0xff);
		//	液位不够提示
		if( hwio.in & HWIO_IBIT_YWG ) DC_TX_SetVisible(3,15,0);
		//	清水阀到位
		if( hwio.in & HWIO_IBIT_QSX ) DC_TX_SetVisible(3,12,0); else DC_TX_SetVisible(3,12,1);
		//	转存阀到位
		if( hwio.in & HWIO_IBIT_ZZX ) DC_TX_SetVisible(3,13,0); else DC_TX_SetVisible(3,13,1);
		//	保存状态
		gs.in = hwio.in;
	}

	//	输出信号
	if( gs.out!=hwio.out ){
		int i = 0;
		//	盐水泵按钮
		i = DC_TX_Put16(6, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hwio.out&HWIO_OBIT_YSB)?1:0, i);
		i = DC_TX_Put16(7, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hwio.out&HWIO_OBIT_YSB)?0:1, i);
		//	电解电源按钮
		i = DC_TX_Put16(8, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hwio.out&HWIO_OBIT_POWER)?1:0, i);
		i = DC_TX_Put16(9, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hwio.out&HWIO_OBIT_POWER)?0:1, i);
		//	多组设置
		DC_TX_MultiSet(3,i);
		//	盐水泵动画
		if( hwio.out & HWIO_OBIT_YSB ) DC_TX_SetVisible(3,11,1); else DC_TX_SetVisible(3,11,0);
		//	电解电源动画
		if( hwio.out & HWIO_OBIT_POWER ) DC_TX_SetVisible(3,4,1); else DC_TX_SetVisible(3,4,0);
		//	保存状态
		gs.out = hwio.out;
	}

	//	输出信号
	if( gs.hso!=hsio.out ){
		int i = 0;
		//	清水阀按钮
		i = DC_TX_Put16(1, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hsio.out&HSIO_OBIT_QS)?1:0, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hsio.out&HSIO_OBIT_QS)?0:1, i);
		//	转存阀按钮
		i = DC_TX_Put16(3, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hsio.out&HSIO_OBIT_ZZ)?1:0, i);
		i = DC_TX_Put16(5, i);
		i = DC_TX_Put16(2, i);
		i = DC_TX_Put16((hsio.out&HSIO_OBIT_ZZ)?0:1, i);
		//	多组设置
		DC_TX_MultiSet(3,i);
		//	保存状态
		gs.hso = hsio.out;
	}

}

static int Page3Input(void)
{
	switch( RX_CONTROL_ID ){
		case 10:	//	返回
			if( RX_CONTROL_TYPE==1 ){		//	按下
				DC_TX_SetIcon(3,10,1);		//	显示按下状态
				hsio.out &= ~(HSIO_OBIT_QS|HSIO_OBIT_ZZ);		//	关闭清水阀和转存阀
				hwio.out &= ~(HWIO_OBIT_YSB|HWIO_OBIT_POWER);	//	关闭盐水泵和电解电源
			}else if( RX_CONTROL_TYPE==0 ){	//	弹起
				DC_TX_SetIcon(3,10,0);		//	显示弹起状态
				return 1;					//	返回画面1
			}
			break;
		case 6:		//	盐水泵打开
			if( RX_CONTROL_TYPE==1 ) hwio.out |= HWIO_OBIT_YSB;
			break;
		case 7:		//	盐水泵关闭
			if( RX_CONTROL_TYPE==1 ) hwio.out &= ~HWIO_OBIT_YSB;
			break;
		case	8:	//	电解电源打开
			if( RX_CONTROL_TYPE==1 ){
				hwio.out |= HWIO_OBIT_POWER;
				if( !(hwio.in & HWIO_IBIT_YWG) ) DC_TX_SetVisible(3,15,1);	//	液位不够提示
			}
			break;
		case	9:	//	电解电源关闭
			if( RX_CONTROL_TYPE==1 ) hwio.out &= ~HWIO_OBIT_POWER;
			break;
		case	1:	//	清水阀打开
			if( RX_CONTROL_TYPE==1 ) hsio.out |= HSIO_OBIT_QS;
			break;
		case	2:	//	清水阀关闭
			if( RX_CONTROL_TYPE==1 ) hsio.out &= ~HSIO_OBIT_QS;
			break;
		case	3:	//	转存阀打开
			if( RX_CONTROL_TYPE==1 ) hsio.out |= HSIO_OBIT_ZZ;
			break;
		case	5:	//	转存阀关闭
			if( RX_CONTROL_TYPE==1 ) hsio.out &= ~HSIO_OBIT_ZZ;
			break;
	}

	//	不退出当前画面
	return -1;
}

int HS_Page_3(void)
{
	printf("%s()\r\n", __func__);

	//	确认进入画面3
	HS_ConfirmScreen(3);

	//	强制更新
	gs.in = gs.out = gs.hso = 0xffffffff;
	Page3Update();

	//	主循环
	for(;;){
		int r;

		HS_Idle();

		//	更新界面
		Page3Update();

		//	查询界面输入
		r = DC_RX_FindCmd();
		if( r>0 && RX_CMD_TYPE==0xB1 && RX_CTRL_MSG==0x26 && RX_SCREEN_ID==3 ){
			r = Page3Input();
			if( r>=0 ) return r;	//	进入其他页面
		}

	}
}

