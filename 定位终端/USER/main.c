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

u8 USART1_TX_BUF[USART2_MAX_RECV_LEN];						 //����1,���ͻ�����
nmea_msg gpsx;												 //GPS��Ϣ
__align(4) u8 dtbuf[50];									 //��ӡ������
const u8 *fixmode_tbl[4] = {"Fail", "Fail", " 2D ", " 3D "}; //fix mode�ַ���

//��ʾGPS��λ��Ϣ
void Gps_Msg_Show(void)
{
	float tp;
	POINT_COLOR = BLUE;
	tp = gpsx.longitude;
	sprintf((char *)dtbuf, "Longitude:%.5f %1c   ", tp /= 100000, gpsx.ewhemi); //�õ������ַ���
	LCD_ShowString(30, 120, 200, 16, 16, dtbuf);
	tp = gpsx.latitude;
	sprintf((char *)dtbuf, "Latitude:%.5f %1c   ", tp /= 100000, gpsx.nshemi); //�õ�γ���ַ���
	LCD_ShowString(30, 140, 200, 16, 16, dtbuf);
	tp = gpsx.altitude;
	sprintf((char *)dtbuf, "Altitude:%.1fm     ", tp /= 10); //�õ��߶��ַ���
	LCD_ShowString(30, 160, 200, 16, 16, dtbuf);
	tp = gpsx.speed;
	sprintf((char *)dtbuf, "Speed:%.3fkm/h     ", tp /= 1000); //�õ��ٶ��ַ���
	LCD_ShowString(30, 180, 200, 16, 16, dtbuf);
	if (gpsx.fixmode <= 3) //��λ״̬
	{
		sprintf((char *)dtbuf, "Fix Mode:%s", fixmode_tbl[gpsx.fixmode]);
		LCD_ShowString(30, 200, 200, 16, 16, dtbuf);
	}
	sprintf((char *)dtbuf, "GPS+BD Valid satellite:%02d", gpsx.posslnum); //���ڶ�λ��GPS������
	LCD_ShowString(30, 220, 200, 16, 16, dtbuf);
	sprintf((char *)dtbuf, "GPS Visible satellite:%02d", gpsx.svnum % 100); //�ɼ�GPS������
	LCD_ShowString(30, 240, 200, 16, 16, dtbuf);

	sprintf((char *)dtbuf, "BD Visible satellite:%02d", gpsx.beidou_svnum % 100); //�ɼ�����������
	LCD_ShowString(30, 260, 200, 16, 16, dtbuf);

	sprintf((char *)dtbuf, "UTC Date:%04d/%02d/%02d   ", gpsx.utc.year, gpsx.utc.month, gpsx.utc.date); //��ʾUTC����
	LCD_ShowString(30, 280, 200, 16, 16, dtbuf);
	sprintf((char *)dtbuf, "UTC Time:%02d:%02d:%02d   ", gpsx.utc.hour, gpsx.utc.min, gpsx.utc.sec); //��ʾUTCʱ��
	LCD_ShowString(30, 300, 200, 16, 16, dtbuf);
}
int main(void)
{
	u16 i, rxlen;
	u16 lenx;
	u8 key = 0XFF;
	u8 upload = 0;
	char *strToSend;
	delay_init();									//��ʱ������ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);								//���ڳ�ʼ��Ϊ115200
	usmart_dev.init(72);							//��ʼ��USMART
	LED_Init();										//��ʼ����LED���ӵ�Ӳ���ӿ�
	KEY_Init();										//��ʼ������
	LCD_Init();										//��ʼ��LCD
	usart3_init(115200);							//��ʼ������3
	usart2_init(38400);								//��ʼ������2
	POINT_COLOR = RED;
	LCD_ShowString(30, 20, 200, 16, 16, "ALIENTEK STM32F1 ^_^");
	LCD_ShowString(30, 40, 200, 16, 16, "S1216F8 GPS TEST");
	LCD_ShowString(30, 60, 200, 16, 16, "ATOM@ALIENTEK");
	LCD_ShowString(30, 80, 200, 16, 16, "KEY0:Upload NMEA Data SW");
	LCD_ShowString(30, 100, 200, 16, 16, "NMEA Data Upload:OFF");
	//GSM����
	connectTCP();
	if (checkConnection() == 1)
	{
		sendString("����");
	}
	//���Խ���
	if (SkyTra_Cfg_Rate(5) != 0) //���ö�λ��Ϣ�����ٶ�Ϊ5Hz,˳���ж�GPSģ���Ƿ���λ.
	{
		LCD_ShowString(30, 120, 200, 16, 16, "SkyTraF8-BD Setting...");
		do
		{
			usart2_init(9600);						   //��ʼ������2������Ϊ9600
			SkyTra_Cfg_Prt(3);						   //��������ģ��Ĳ�����Ϊ38400
			usart2_init(38400);						   //��ʼ������2������Ϊ38400
			key = SkyTra_Cfg_Tp(100000);			   //������Ϊ100ms
		} while (SkyTra_Cfg_Rate(5) != 0 && key != 0); //����SkyTraF8-BD�ĸ�������Ϊ5Hz
		LCD_ShowString(30, 120, 200, 16, 16, "SkyTraF8-BD Set Done!!");
		delay_ms(500);
		LCD_Fill(30, 120, 30 + 200, 120 + 16, WHITE); //�����ʾ
	}
	while (1)
	{
		delay_ms(1);
		if (USART2_RX_STA & 0X8000) //���յ�һ��������
		{
			rxlen = USART2_RX_STA & 0X7FFF; //�õ����ݳ���
			for (i = 0; i < rxlen; i++)
				USART1_TX_BUF[i] = USART2_RX_BUF[i];
			USART2_RX_STA = 0;						  //������һ�ν���
			USART1_TX_BUF[i] = 0;					  //�Զ���ӽ�����
			GPS_Analysis(&gpsx, (u8 *)USART1_TX_BUF); //�����ַ���
			Gps_Msg_Show();							  //��ʾ��Ϣ
			sprintf((char *)strToSend, "Longitude:%.5f  Latitude:%.5f  ", ((float)gpsx.longitude) / 100000, ((float)gpsx.latitude) / 100000);
			sendString(strToSend); //���ͽ��յ������ݵ�GPRS
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
