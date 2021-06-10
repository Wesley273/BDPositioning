#ifndef __SIM800C_H__
#define __SIM800C_H__
#include "sys.h"

#define swap16(x) (x & 0XFF) << 8 | (x & 0XFF00) >> 8 //高低字节交换宏定义

extern u8 Scan_Wtime;

void sim_at_response(u8 mode);
u8 *sim800c_check_cmd(u8 *str);
u8 sim800c_send_cmd(u8 *cmd, u8 *ack, u16 waittime);
void connectTCP();
u8 sendString(char *str);
u8 checkConnection();
#endif
