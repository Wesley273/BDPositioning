#include "sim800c.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "usart3.h"
#include "string.h"
#include "timer.h"

u8 Scan_Wtime = 0;	 //��������ɨ����Ҫ��ʱ��
u8 BT_Scan_mode = 0; //����ɨ���豸ģʽ��־

//usmart֧�ֲ���
//���յ���ATָ��Ӧ�����ݷ��ظ����Դ���
//mode:0,������USART3_RX_STA;
//     1,����USART3_RX_STA;
void sim_at_response(u8 mode)
{
	if (USART3_RX_STA & 0X8000) //���յ�һ��������
	{
		USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //��ӽ�����
		printf("%s", USART3_RX_BUF);			   //���͵�����
		if (mode)
			USART3_RX_STA = 0;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//ATK-SIM800C �������(���Ų��ԡ����Ų��ԡ�GPRS���ԡ���������)���ô���

//SIM800C���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����
//����,�ڴ�Ӧ������λ��(str��λ��)
u8 *sim800c_check_cmd(u8 *str)
{
	char *strx = 0;
	if (USART3_RX_STA & 0X8000) //���յ�һ��������
	{
		u8 *p2;
		USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //��ӽ�����
		strx = strstr((const char *)USART3_RX_BUF, (const char *)str);
	}
	return (u8 *)strx;
}
//SIM800C��������
//cmd:���͵������ַ���(����Ҫ��ӻس���),��cmd<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 sim800c_send_cmd(u8 *cmd, u8 *ack, u16 waittime)
{
	u8 res = 0;
	USART3_RX_STA = 0;
	if ((u32)cmd <= 0XFF)
	{
		while ((USART3->SR & 0X40) == 0)
			; //�ȴ���һ�����ݷ������
		USART3->DR = (u32)cmd;
	}
	else
		u3_printf("%s\r\n", cmd); //��������

	if (waittime == 1100) //11s����ش�������(����ɨ��ģʽ)
	{
		Scan_Wtime = 11;   //��Ҫ��ʱ��ʱ��
		TIM6_SetARR(9999); //����1S��ʱ�ж�
	}
	if (ack && waittime) //��Ҫ�ȴ�Ӧ��
	{
		while (--waittime) //�ȴ�����ʱ
		{
			if (BT_Scan_mode) //����ɨ��ģʽ
			{
				res = KEY_Scan(0); //������һ��
				if (res == WKUP_PRES)
					return 2;
			}
			delay_ms(10);
			if (USART3_RX_STA & 0X8000) //���յ��ڴ���Ӧ����
			{
				if (sim800c_check_cmd(ack))
					break; //�õ���Ч����
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
	u8 connectsta = 0;							 //0,��������;1,���ӳɹ�;2,���ӹر�;
	sim800c_send_cmd("AT+CIPSTATUS", "OK", 500); //�������״̬
	if (strstr((const char *)USART3_RX_BUF, "CLOSED"))
		connectsta = 2;
	if (strstr((const char *)USART3_RX_BUF, "CONNECT OK"))
		connectsta = 1;
	return connectsta;
}
void connectTCP()
{
	u8 connectsta = 0;											   //0,��������;1,���ӳɹ�;2,���ӹر�;
	sim800c_send_cmd("AT+CGCLASS=\"B\"", "OK", 1000);			   //����GPRS�ƶ�̨���ΪB,֧�ְ����������ݽ���
	sim800c_send_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"", "OK", 1000); //����PDP������,��������Э��,��������Ϣ
	sim800c_send_cmd("AT+CGATT=1", "OK", 500);					   //����GPRSҵ��
	sim800c_send_cmd("AT+CIPCSGP=1,\"CMNET\"", "OK", 500);		   //����ΪGPRS����ģʽ
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
	if (sim800c_send_cmd("AT+CIPSEND", ">", 500) == 0) //��������
	{
		u3_printf("%s\r\n", str);
		delay_ms(10);
		if (sim800c_send_cmd((u8 *)0X1A, "SEND OK", 1000) == 0)
		{
			//LCD_ShowString(30, 80, 200, 16, 16, "Send OK!"); //��ȴ�10s
			res = 1;
		}
		else
			//LCD_ShowString(30, 80, 200, 16, 16, "Send Fail");
		delay_ms(1000);
	}
	else
		sim800c_send_cmd((u8 *)0X1B, 0, 0); //ESC,ȡ������
	return res;
}