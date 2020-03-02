/*
 * tcp_server.c
 *
 *  Created on: Feb 12, 2020
 *      Author: ierturk
 */

#include <string.h>
#include "stdio.h"
#include <sys/param.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "uart_drv.h"

#define PORT 8998

extern osMessageQueueId_t tiva_msgHandle;

static void do_retransmit(const int sock)
{
    int len;
    tiva_msg_t msg;

    while(1) {
        do {
    		len = -1;
        	if(osMessageQueueGet(tiva_msgHandle, &msg, NULL, 0U) == osOK) {
    			len = msg.len;
    		}

        	if (len < 0) {
        		//
            } else if (len == 0) {
                // printf("Connection closed");
            } else {
    			int to_write = len;
    			while (to_write > 0) {
    				int written = send(sock, msg.buff + (len - to_write), to_write, 0);
    				if (written < 0) {
    					// printf("Error occurred during sending");
    				}
    				to_write -= written;
    			}
            }
        } while (len > 0);
    }
}

void tcp_svc(void)
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
        vTaskDelete(NULL);
        return;
    }

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        goto CLEAN_UP;
    }

    err = listen(listen_sock, 1);
    if (err != 0) {
        goto CLEAN_UP;
    }


    while (1) {

        // printf("TCP socket listening on port 8998 !...");

        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            break;
        }
        if (source_addr.sin_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        }
        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

