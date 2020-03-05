/*
 * main_app.h
 *
 *  Created on: Feb 12, 2020
 *      Author: ierturk
 */

#ifndef SRC_MAINAPP_H_
#define SRC_MAINAPP_H_

#define UART6_DMA_RX_BUFF_SIZE (1024)

#include "stdint.h"

typedef struct {
	uint8_t *buff;
	uint16_t len;
	uint32_t id;
} tiva_msg_t;

void init_uart_drv(void);

#endif /* SRC_MAINAPP_H_ */
