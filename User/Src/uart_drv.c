/*
 * AppMain.cpp
 *
 *  Created on: Feb 12, 2020
 *      Author: ierturk
 */

#include <stdint.h>
#include <uart_drv.h>
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "usart.h"

extern DMA_HandleTypeDef hdma_usart6_rx;
extern UART_HandleTypeDef huart6;
static uint8_t rxBuff[UART6_DMA_RX_BUFF_SIZE] = {};
// static uint8_t *txBuff;
static tiva_msg_t serBuff;

QueueHandle_t serBuffQueueHandle = NULL;

void init_uart_drv(void) {

	serBuffQueueHandle = xQueueCreate(16, sizeof(tiva_msg_t));
	__HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);

	HAL_UART_Receive_DMA(&huart6, (uint8_t*)rxBuff, UART6_DMA_RX_BUFF_SIZE);
	// __HAL_DMA_DISABLE_IT(&hdma_usart6_rx, DMA_IT_HT);
	// __HAL_DMA_ENABLE_IT(&hdma_usart6_rx, DMA1_IT_HT);
	// __HAL_DMA_ENABLE_IT(&hdma_usart6_rx, DMA_IT_TC);
}

 void uart6_dma_usart_rx_check(void) {
     static size_t old_pos;
     size_t pos;

     // Calculate current position in buffer
     pos = UART6_DMA_RX_BUFF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart6_rx);

     // Check change in received data
     if (pos != old_pos) {
     	// Current position is over previous one
         if (pos > old_pos) {
             // We are in "linear" mode
             // Process data directly by subtracting "pointers"
        	 // uart6dma.rx.cb(&uart6dma.rx.buffer[old_pos], pos - old_pos);
        	 // HAL_UART_Transmit_DMA(&huart6, &rxBuff[old_pos], pos - old_pos);
        	 serBuff.len = pos - old_pos;
        	 serBuff.buff = &rxBuff[old_pos];
        	 xQueueSendFromISR(serBuffQueueHandle, &serBuff, NULL);

         } else {
        	 // We are in "overflow" mode
             // First process data to the end of buffer
        	 // uart6dma.rx.cb(&uart6dma.rx.buffer[old_pos], sizeof(uart6dma.rx.buffer) - old_pos);
        	 // HAL_UART_Transmit_DMA(&huart6, &rxBuff[old_pos], UART6_DMA_RX_BUFF_SIZE - old_pos);
        	 serBuff.len = UART6_DMA_RX_BUFF_SIZE - old_pos;
        	 serBuff.buff = &rxBuff[old_pos];
        	 xQueueSendFromISR(serBuffQueueHandle, &serBuff, NULL);

             // Check and continue with beginning of buffer
             if (pos) {
            	 // uart6dma.rx.cb(&uart6dma.rx.buffer[0], pos);
            	 // HAL_UART_Transmit_DMA(&huart6, &rxBuff[0], pos);
            	 serBuff.len = pos;
            	 serBuff.buff = &rxBuff[0];
            	 xQueueSendFromISR(serBuffQueueHandle, &serBuff, NULL);

             }
         }
     }
     // Save current position as old
     old_pos = pos;

     // Check and manually update if we reached end of buffer
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


