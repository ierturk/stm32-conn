/*
 * AppMain.cpp
 *
 *  Created on: Feb 12, 2020
 *      Author: ierturk
 */

#include <stdint.h>
#include "cmsis_os.h"
#include "uart_drv.h"
#include "usart.h"

extern DMA_HandleTypeDef hdma_usart6_rx;
extern UART_HandleTypeDef huart6;
extern osMessageQueueId_t tiva_msgHandle;

static uint8_t rxBuff[UART6_DMA_RX_BUFF_SIZE] = {};

void init_uart_drv(void) {

	__HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart6, (uint8_t*)rxBuff, UART6_DMA_RX_BUFF_SIZE);

}

 void uart6_dma_usart_rx_check(void) {
     static uint32_t msg_id = 0;
	 static size_t old_pos;
     size_t pos;
     tiva_msg_t msg;

     pos = UART6_DMA_RX_BUFF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart6_rx);

     if (pos != old_pos) {
         if (pos > old_pos) {
        	 msg.len = pos - old_pos;
        	 msg.buff = &rxBuff[old_pos];
        	 msg.id = msg_id++;
        	 osMessageQueuePut(tiva_msgHandle, &msg, 0U, 0U);


         } else {
        	 msg.len = UART6_DMA_RX_BUFF_SIZE - old_pos;
        	 msg.buff = &rxBuff[old_pos];
        	 msg.id = msg_id++;
        	 osMessageQueuePut(tiva_msgHandle, &msg, 0U, 0U);

             if (pos) {
            	 msg.len = pos;
            	 msg.buff = &rxBuff[0];
            	 msg.id = msg_id++;
            	 osMessageQueuePut(tiva_msgHandle, &msg, 0U, 0U);
             }
         }
     }
     old_pos = pos;

     if (old_pos == UART6_DMA_RX_BUFF_SIZE) {
         old_pos = 0;
     }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	uart6_dma_usart_rx_check();
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
	uart6_dma_usart_rx_check();
}

