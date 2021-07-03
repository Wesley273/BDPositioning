#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "usmart.h"
#include "gps.h"
#include "usart3.h"
#include "usart2.h"
#include "key.h"
#include "sim800c.h"
#include "string.h"

u8 USART1_TX_BUF[USART2_MAX_RECV_LEN];						 //串口1,发送缓存区
nmea_msg gpsx;												 //GPS信息
__align(4) u8 dtbuf[50];									 //打印缓存器
const u8 *fixmode_tbl[4] = {"Fail", "Fail", " 2D ", " 3D "}; //fix mode字符串

//显示GPS定位信息
void Gps_Msg_Show(void)
{
	float tp;
	POINT_COLOR = BLUE;
	tp = gpsx.longitude;
	sprintf((char *)dtbuf, "Longitude:%.5f %1c   ", tp /= 100000, gpsx.ewhemi); //得到经度字符串
	LCD_ShowString(30, 120, 200, 16, 16, dtbuf);
	tp = gpsx.latitude;
	sprintf((char *)dtbuf, "Latitude:%.5f %1c   ", tp /= 100000, gpsx.nshemi); //得到纬度字符串
	LCD_ShowString(30, 140, 200, 16, 16, dtbuf);
	tp = gpsx.altitude;
	sprintf((char *)dtbuf, "Altitude:%.1fm     ", tp /= 10); //得到高度字符串
	LCD_ShowString(30, 160, 200, 16, 16, dtbuf);
	tp = gpsx.speed;
	sprintf((char *)dtbuf, "Speed:%.3fkm/h     ", tp /= 1000); //得到速度字符串
	LCD_ShowString(30, 180, 200, 16, 16, dtbuf);
	if (gpsx.fixmode <= 3) //定位状态
	{
		sprintf((char *)dtbuf, "Fix Mode:%s", fixmode_tbl[gpsx.fixmode]);
		LCD_ShowString(30, 200, 200, 16, 16, dtbuf);
	}
	sprintf((char *)dtbuf, "GPS+BD Valid satellite:%02d", gpsx.posslnum); //用于定位的GPS卫星数
	LCD_ShowString(30, 220, 200, 16, 16, dtbuf);
	sprintf((char *)dtbuf, "GPS Visible satellite:%02d", gpsx.svnum % 100); //可见GPS卫星数
	LCD_ShowString(30, 240, 200, 16, 16, dtbuf);

	sprintf((char *)dtbuf, "BD Visible satellite:%02d", gpsx.beidou_svnum % 100); //可见北斗卫星数
	LCD_ShowString(30, 260, 200, 16, 16, dtbuf);

	sprintf((char *)dtbuf, "UTC Date:%04d/%02d/%02d   ", gpsx.utc.year, gpsx.utc.month, gpsx.utc.date); //显示UTC日期
	LCD_ShowString(30, 280, 200, 16, 16, dtbuf);
	sprintf((char *)dtbuf, "UTC Time:%02d:%02d:%02d   ", gpsx.utc.hour, gpsx.utc.min, gpsx.utc.sec); //显示UTC时间
	LCD_ShowString(30, 300, 200, 16, 16, dtbuf);
}
int main(void)
{
	u16 i, rxlen;
	u16 lenx;
	u8 key = 0XFF;
	u8 upload = 0;
	char *strToSend;
	delay_init();									//延时函数初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);								//串口初始化为115200
	usmart_dev.init(72);							//初始化USMART
	LED_Init();										//初始化与LED连接的硬件接口
	KEY_Init();										//初始化按键
	LCD_Init();										//初始化LCD
	usart3_init(115200);							//初始化串口3
	usart2_init(38400);								//初始化串口2
	POINT_COLOR = RED;
	LCD_ShowString(30, 20, 200, 16, 16, "ALIENTEK STM32F1 ^_^");
	LCD_ShowString(30, 40, 200, 16, 16, "S1216F8 GPS TEST");
	LCD_ShowString(30, 60, 200, 16, 16, "ATOM@ALIENTEK");
	LCD_ShowString(30, 80, 200, 16, 16, "KEY0:Upload NMEA Data SW");
	LCD_ShowString(30, 100, 200, 16, 16, "NMEA Data Upload:OFF");
	//GSM测试
	connectTCP();
	if (checkConnection() == 1)
	{
		sendString("测试");
	}
	//测试结束
	if (SkyTra_Cfg_Rate(5) != 0) //设置定位信息更新速度为5Hz,顺便判断GPS模块是否在位.
	{
		LCD_ShowString(30, 120, 200, 16, 16, "SkyTraF8-BD Setting...");
		do
		{
			usart2_init(9600);						   //初始化串口2波特率为9600
			SkyTra_Cfg_Prt(3);						   //重新设置模块的波特率为38400
			usart2_init(38400);						   //初始化串口2波特率为38400
			key = SkyTra_Cfg_Tp(100000);			   //脉冲宽度为100ms
		} while (SkyTra_Cfg_Rate(5) != 0 && key != 0); //配置SkyTraF8-BD的更新速率为5Hz
		LCD_ShowString(30, 120, 200, 16, 16, "SkyTraF8-BD Set Done!!");
		delay_ms(500);
		LCD_Fill(30, 120, 30 + 200, 120 + 16, WHITE); //清除显示
	}
	while (1)
	{
		delay_ms(1);
		if (USART2_RX_STA & 0X8000) //接收到一次数据了
		{
			rxlen = USART2_RX_STA & 0X7FFF; //得到数据长度
			for (i = 0; i < rxlen; i++)
				USART1_TX_BUF[i] = USART2_RX_BUF[i];
			USART2_RX_STA = 0;						  //启动下一次接收
			USART1_TX_BUF[i] = 0;					  //自动添加结束符
			GPS_Analysis(&gpsx, (u8 *)USART1_TX_BUF); //分析字符串
			Gps_Msg_Show();							  //显示信息
			sprintf((char *)strToSend, "Longitude:%.5f  Latitude:%.5f  ", ((float)gpsx.longitude) / 100000, ((float)gpsx.latitude) / 100000);
			sendString(strToSend); //发送接收到的数据到GPRS
			delay_ms(1000);
		}
		key = KEY_Scan(0);
		if (key == KEY0_PRES)
		{
			upload = !upload;
			POINT_COLOR = RED;
			if (upload)
				LCD_ShowString(30, 100, 200, 16, 16, "NMEA Data Upload:ON ");
			else
				LCD_ShowString(30, 100, 200, 16, 16, "NMEA Data Upload:OFF");
		}
		if ((lenx % 500) == 0)
			LED0 = !LED0;
		lenx++;
	}
}
