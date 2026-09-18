#ifndef __MODBUS_TCP_SERVER_H__
#define __MODBUS_TCP_SERVER_H__

#include <stdint.h>
#include "w5500.h"

void modbus_tcp_serv_init(w5500_t* c);
void modbus_tcp_serv_process(void);

#endif