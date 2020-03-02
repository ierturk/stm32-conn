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
#include "uart_drv.h"

#define UDP_PORT 8997

extern osMessageQueueId_t tiva_msgHandle;

void udp_svc(void)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    tiva_msg_t msg;

    // printf("Started listen UDP from port 8997!\r\n");

    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(UDP_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

/*
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
        	// printf("Can not open UDP socket!");
            break;
        }
*/
        int sock = -1;
        do {
        	sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        } while(sock < 0);

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
        	// printf("Can not bind UDP socket!");
        }

        while (1) {

            struct sockaddr_in source_addr;
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            if (len < 0) {
                break;
            }
            else {
                if (source_addr.sin_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }


                while(1) {
					if(osMessageQueueGet(tiva_msgHandle, &msg, NULL, 0U) == osOK) {

						int err = sendto(sock, msg.buff, msg.len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
						if (err < 0) {
							break;
						}
					}
                }

            }
        }

        if (sock != -1) {
            shutdown(sock, 0);
            close(sock);
        }
    }
    // vTaskDelete(NULL);
    osThreadTerminate(NULL);
}
