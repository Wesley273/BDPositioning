#include "sim800c.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "usart3.h"
#include "string.h"
#include "timer.h"

u8 Scan_Wtime = 0;	 //保存蓝牙扫描需要的时间
u8 BT_Scan_mode = 0; //蓝牙扫描设备模式标志

//usmart支持部分
//将收到的AT指令应答数据返回给电脑串口
//mode:0,不清零USART3_RX_STA;
//     1,清零USART3_RX_STA;
void sim_at_response(u8 mode)
{
	if (USART3_RX_STA & 0X8000) //接收到一次数据了
	{
		USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //添加结束符
		printf("%s", USART3_RX_BUF);			   //发送到串口
		if (mode)
			USART3_RX_STA = 0;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//ATK-SIM800C 各项测试(拨号测试、短信测试、GPRS测试、蓝牙测试)共用代码

//SIM800C发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//其他,期待应答结果的位置(str的位置)
u8 *sim800c_check_cmd(u8 *str)
{
	char *strx = 0;
	if (USART3_RX_STA & 0X8000) //接收到一次数据了
	{
		u8 *p2;
		USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //添加结束符
		strx = strstr((const char *)USART3_RX_BUF, (const char *)str);
	}
	return (u8 *)strx;
}
//SIM800C发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 sim800c_send_cmd(u8 *cmd, u8 *ack, u16 waittime)
{
	u8 res = 0;
	USART3_RX_STA = 0;
	if ((u32)cmd <= 0XFF)
	{
		while ((USART3->SR & 0X40) == 0)
			; //等待上一次数据发送完成
		USART3->DR = (u32)cmd;
	}
	else
		u3_printf("%s\r\n", cmd); //发送命令

	if (waittime == 1100) //11s后读回串口数据(蓝牙扫描模式)
	{
		Scan_Wtime = 11;   //需要定时的时间
		TIM6_SetARR(9999); //产生1S定时中断
	}
	if (ack && waittime) //需要等待应答
	{
		while (--waittime) //等待倒计时
		{
			if (BT_Scan_mode) //蓝牙扫描模式
			{
				res = KEY_Scan(0); //返回上一级
				if (res == WKUP_PRES)
					return 2;
			}
			delay_ms(10);
			if (USART3_RX_STA & 0X8000) //接收到期待的应答结果
			{
				if (sim800c_check_cmd(ack))
					break; //得到有效数据
				USART3_RX_STA = 0;
			}
		}
		if (waittime == 0)
			res = 1;
	}
	return res;
}
u8 checkConnection()
{
	u8 connectsta = 0;							 //0,正在连接;1,连接成功;2,连接关闭;
	sim800c_send_cmd("AT+CIPSTATUS", "OK", 500); //检查连接状态
	if (strstr((const char *)USART3_RX_BUF, "CLOSED"))
		connectsta = 2;
	if (strstr((const char *)USART3_RX_BUF, "CONNECT OK"))
		connectsta = 1;
	return connectsta;
}
void connectTCP()
{
	u8 connectsta = 0;											   //0,正在连接;1,连接成功;2,连接关闭;
	sim800c_send_cmd("AT+CGCLASS=\"B\"", "OK", 1000);			   //设置GPRS移动台类别为B,支持包交换和数据交换
	sim800c_send_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"", "OK", 1000); //设置PDP上下文,互联网接协议,接入点等信息
	sim800c_send_cmd("AT+CGATT=1", "OK", 500);					   //附着GPRS业务
	sim800c_send_cmd("AT+CIPCSGP=1,\"CMNET\"", "OK", 500);		   //设置为GPRS连接模式
	sim800c_send_cmd("AT+CLPORT=\"TCP\",\"2000\"", "OK", 500);
	sim800c_send_cmd("AT+CIPSTART=\"TCP\",\"106.14.150.177\",\"8086\"", "OK", 500);
	if (checkConnection() == 2)
	{
		sim800c_send_cmd("AT+CIPSTART=\"TCP\",\"106.14.150.177\",\"8086\"", "OK", 500);
	}
}

u8 sendString(char *str)
{
	u8 res = 0;
	if (sim800c_send_cmd("AT+CIPSEND", ">", 500) == 0) //发送数据
	{
		u3_printf("%s\r\n", str);
		delay_ms(10);
		if (sim800c_send_cmd((u8 *)0X1A, "SEND OK", 1000) == 0)
		{
			//LCD_ShowString(30, 80, 200, 16, 16, "Send OK!"); //最长等待10s
			res = 1;
		}
		else
			//LCD_ShowString(30, 80, 200, 16, 16, "Send Fail");
		delay_ms(1000);
	}
	else
		sim800c_send_cmd((u8 *)0X1B, 0, 0); //ESC,取消发送
	return res;
}