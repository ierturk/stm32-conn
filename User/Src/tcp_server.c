/*
 * tcp_server.c
 *
 *  Created on: Feb 12, 2020
 *      Author: ierturk
 */

#include <string.h>
#include <sys/param.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <uart_drv.h>
#include "queue.h"

#define PORT 8998
#define TCP_SERVER_THREAD_PRIO  ( tskIDLE_PRIORITY + 5 )

tiva_msg_t tcpUartBuff = {};
extern QueueHandle_t serBuffQueueHandle;
extern UART_HandleTypeDef huart6;

static void do_retransmit(const int sock)
{
    int len;
    int ret;
    char rx_buffer[128];


    while (1) {
    	ret = recv(sock, NULL, 1, MSG_PEEK | MSG_DONTWAIT);
    	if(ret == 0) {
    		break;
    	} else {
    		len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
    		if(len == 0) {
    			break;
    		} else if (len > 0) {
    			// HAL_UART_Transmit_DMA(&huart6, (uint8_t *)rx_buffer, len);
    		}

        	if(xQueueReceive(serBuffQueueHandle, &tcpUartBuff, portMAX_DELAY)) {
    			len = tcpUartBuff.len;
/*
    			if(len > 0) {
    				int to_write = len;
    				while (to_write > 0) {
    					int written = send(sock, tcpUartBuff.buff + (len - to_write), to_write, 0);
    					if (written < 0) {
    						// LOGE(TAG, "Error occurred during sending: errno %d", errno);
    					}
    					to_write -= written;
    				}
    	        }
*/
    		}
    	}
    }


/*
    do {
        // len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
		len = -1;
    	if(xQueueReceive(serBuffQueueHandle, &tcpUartBuff, portMAX_DELAY)) {
			len = tcpUartBuff.len;
			// tcpUartBuff.buff[len] = 0;
		}

		if (len < 0) {
            // ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            // ESP_LOGW(TAG, "Connection closed");
        } else {
			int to_write = len;
			while (to_write > 0) {
				int written = send(sock, tcpUartBuff.buff + (len - to_write), to_write, 0);
				if (written < 0) {
					// ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
				}
				to_write -= written;
			}
        }
    } while (len > 0);
*/
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;


    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        // LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    // LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        // ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    // LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        // ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        // LOGI(TAG, "Socket listening");

        struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            // LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Convert ip address to string
        if (source_addr.sin_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        }
        // LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void tcp_server_thread_init(void)
{
    sys_thread_new("tcp_server_task", tcp_server_task, NULL, DEFAULT_THREAD_STACKSIZE, TCP_SERVER_THREAD_PRIO);
}
