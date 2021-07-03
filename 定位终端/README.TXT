实验器材:
	战舰STM32F103开发板V3版本/精英STM32F103开发板V1版本
	
实验目的:
	学习ATK-S1216F8-BD模块的使用，实现GPS/北斗定位
	
硬件资源:
	1,DS0(连接在PB5),DS1(连接在PE5)
	2,串口1(波特率:115200,PA9/PA10连接在板载USB转串口芯片CH340上面)
	3,串口3(波特率:38400,PB10/PB11接ATK MODULE接口)
	4,按键KEY0(PE4)
	5,ALIENTEK 2.8/3.5/4.3/7寸TFTLCD模块(通过FSMC驱动,FSMC_NE4接LCD片选/A10接RS) 
	6,ATK MODULE接口&ATK-S1216F8-BD(模块直接插ATK MODULE接口即可),连接关系如下:
	STM32开发板-->ATK-S1216F8-BD模块 
		   PA4-->PPS
		   PB10-->RXD
		   PB11-->TXD
		   GND-->GND
		   5V-->VCC 
	
实验现象:
	通过串口3连接ATK-S1216F8-BD模块，然后通过液晶显示GPS信息，包括精度、纬度、高度、速度、用于定位的卫星数、可见卫星
	数、UTC时间等信息。同时，可以通过USMART工具，设置GPS模块的刷新速率（最大支持20Hz刷新）和时钟脉冲的配置。另外，通过
	KEY0按键，可以开启或关闭NMEA数据的上传（即输出到串口1，方便开发调试）。


注意事项:
	1,4.3寸和7寸屏需要比较大电流,USB供电可能不足,请用外部电源适配器(推荐外接12V 1A电源).
	2,本例程在LCD_Init函数里面(在ILI93xx.c),用到了printf,如果不初始化串口1,将导致液晶无法显示!!
	3,USB串口波特率为115200
	4,请用USB线连接在USB_232,找到USB转串口后测试本例程.
	5,P3(精英板)/P4(战舰V3)的PA9/PA10必须通过跳线帽连接在RXD/TXD上. 
	6,如果是战舰V3，则必需用跳线帽短接开发板P8的PB10(TX)和GBC_RX以及PB11(RX)和GBC_TX
	7,如有其他问题，请参考<ATK-S1216F8-BD模块问题汇总.pdf>（暂时没有这个文档）。
	8,本例程详细说明，见光盘：9，增值资料\1，ALIENTEK 产品资料\16，ATK-S1216F8-BD模块资料\ATK-S1216F8-BD模块使用说明_AN1605B.pdf



			正点原子@ALIENTEK 
			广州市星翼电子科技有限公司
			电话：020-38271790
			传真：020-36773971
			购买：http://eboard.taobao.com
			公司网站：www.alientek.com
			技术论坛：www.openedv.com



































