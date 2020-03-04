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

#include "lwip/api.h"
#include "lwip/ip.h"

#define PORT 8998

extern osMessageQueueId_t tiva_msgHandle;

static void do_retransmit(const int sock)
{
    int len, ret;
    char rx_buffer[128];
	// int ret;
    tiva_msg_t msg;
    u32_t ticks_a, ticks_b;

/*
    while ((ret = recv(sock, NULL, 1, MSG_PEEK | MSG_DONTWAIT)) != 0) {
        // sleep(rand() % 2); // Sleep for a bit to avoid spam
        // fflush(stdin);
        // printf("I am alive: %d\n", socket);
    	osDelay(1000);
    	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
    	if(len > 0) {
    		ticks_b = sys_now();
    	}
    	osDelay(1000);
    }
*/



    do {
    	ticks_a = sys_now();

    	ret = recv(sock, NULL, 1, MSG_PEEK | MSG_DONTWAIT);
    	if(ret <= 0) {
    		break;
    	}

    	len = 1;


        // len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_PEEK | MSG_DONTWAIT);

/*
        while ((len = recv(sock, NULL, 1, MSG_PEEK | MSG_DONTWAIT)) != 0) {
            // sleep(rand() % 2); // Sleep for a bit to avoid spam
            // fflush(stdin);
            // printf("I am alive: %d\n", socket);
        	osDelay(1);
        	// len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
        	// if(len > 0) {
        	//	ticks_b = sys_now();
        	// }
        }
*/
/*
        // len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
        ticks_b = sys_now();

        if (len < 0) {
            // LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        	len = 128;
        } else if (len == 0) {
            // LOGW(TAG, "Connection closed");
        	break;
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            // LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation.

            int to_write = len;
            while (to_write > 0) {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                if (written < 0) {
                    // LOGE(TAG, "Error occurred during sending: errno %d", errno);
                }
                to_write -= written;
            }


        	if(osMessageQueueGet(tiva_msgHandle, &msg, NULL, 0U) == osOK) {
    			len = msg.len;
                int to_write = len;
                while (to_write > 0) {
                    int written = send(sock, msg.buff + (len - to_write), to_write, 0);
                    if (written < 0) {
                        // LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    	len = -1;
                    	break;
                    }
                    to_write -= written;
                }
            }

        }
*/
    } while (len > 0);
}


static void tcp_svc(void *arg)
{
	LWIP_UNUSED_ARG(arg);


	struct lwip_sock *tcp_sock;
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
        // vTaskDelete(NULL);
    	goto CLEAN_UP;
    	return;
    }

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        goto CLEAN_UP;
    }

    err = listen(listen_sock, 10);
    if (err != 0) {
        goto CLEAN_UP;
    }


    while (1) {

        // printf("TCP socket listening on port 8998 !...");

        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            continue;
        }
        if (source_addr.sin_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        }

        // fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
        // tcp_sock =     get_socket(sock);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
	shutdown(listen_sock, 0);
    close(listen_sock);
    // vTaskDelete(NULL);
    osThreadTerminate(NULL);
    // return;
}



static void tcp_server_netconn_thread(void *arg)
{
	LWIP_UNUSED_ARG(arg);

	struct netconn *conn, *newconn;
	err_t err;

	tiva_msg_t msg;

	/* create a connection structure */
	conn = netconn_new(NETCONN_TCP);

	/* bind the connection to port 2000 on any local IP address */
	netconn_bind(conn, NULL, PORT);

	// printf("Now listening\n");
	/* tell the connection to listen for incoming connection requests */
	netconn_listen(conn);

	/* Grab new connection. */
	err = netconn_accept(conn, &newconn);
	// printf("accepted new connection %p\n", newconn);

	/* Process the new connection. */
	if (err == ERR_OK)
	{
		// struct netbuf *buf;
		// void *data;
		// u16_t len;

		// netconn_set_recvtimeout(newconn, 10000);
		// err = netconn_recv(newconn, &buf);
		// int optval = 1;
		// setsockopt(net, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
		// newconn->pcb.tcp->so_options |= SOF_KEEPALIVE;
		// ((struct tcp_pcb *)newconn->pcb.tcp)->so_options = |= SOF_KEEPALIVE;

		while(1) {
	    	if(osMessageQueueGet(tiva_msgHandle, &msg, NULL, 0U) == osOK) {
				// do{
					// netbuf_data(buf, &data, &len);
					err = netconn_write(newconn, msg.buff, msg.len, NETCONN_COPY);
					if (err != ERR_OK) {
						// printf("tcpecho: netconn_write: error \"%s\"\n", lwip_strerr(err));
					}
				// } while (netbuf_next(buf) >= 0);
			}
		}
/*
		while ((err = netconn_recv(newconn, &buf)) == ERR_OK)
		{
			// printf("Received data\n");
			do{
				netbuf_data(buf, &data, &len);
				err = netconn_write(newconn, data, len, NETCONN_COPY);
				if (err != ERR_OK) {
					// printf("tcpecho: netconn_write: error \"%s\"\n", lwip_strerr(err));
				}
			} while (netbuf_next(buf) >= 0);

			netbuf_delete(buf);
		}
*/
	/* Close connection and discard connection identifier. */
	netconn_close(newconn);
	netconn_delete(newconn);
	}
}

void tcp_server_netconn_init(void)
{
  sys_thread_new("tcp_svc", tcp_svc, NULL, 1024, 5);
}

