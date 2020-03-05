/*
 * udp_server.c
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

#define PORT 8997
#define UDP_SERVER_THREAD_PRIO  ( tskIDLE_PRIORITY + 5 )

tiva_msg_t udpUartBuff = {};
extern QueueHandle_t serBuffQueueHandle;

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);


        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            // LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        // LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            // LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        // LOGI(TAG, "Socket bound, port %d", PORT);

        while (1) {

            // LOGI(TAG, "Waiting for data");
            struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                // LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                if (source_addr.sin_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }


    			while(xQueueReceive(serBuffQueueHandle, &udpUartBuff, portMAX_DELAY)) {
    				// len = udpUartBuff.len;
    				// udpUartBuff.buff[len] = 0;

                    int err = sendto(sock, udpUartBuff.buff, udpUartBuff.len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                    if (err < 0) {
                        // LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
    			}
    			/*
    			else {
                    rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                    // LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                    // LOGI(TAG, "%s", rx_buffer);

                    int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                    if (err < 0) {
                        // ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
    			}
    			*/
            }
        }

        if (sock != -1) {
            // LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void udp_server_thread_init(void)
{
    sys_thread_new("udp_server_task", udp_server_task, NULL, DEFAULT_THREAD_STACKSIZE, UDP_SERVER_THREAD_PRIO);
}
