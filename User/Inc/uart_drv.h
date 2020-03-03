/*
 * uar_drv.h
 *
 *  Created on: Feb 12, 2020
 *      Author: ierturk
 */

#ifndef UART_DRV_H_
#define UART_DRV_H_

#define UART6_DMA_RX_BUFF_SIZE (4096)

typedef struct {
	uint8_t *buff;
	uint16_t len;
	uint32_t id;
} tiva_msg_t;

void init_uart_drv(void);

#endif /* UART_DRV_H_ */
