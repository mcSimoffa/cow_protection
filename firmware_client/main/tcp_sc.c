#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
//#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "esp_err.h"

#include "tcp_sc.h"

//-----------------------------------------------------------------
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#define PORT CONFIG_EXAMPLE_PORT
#define PAYLOAD 512
//-----------------------------------------------------------------
static const char *TAG = "TCP_SC";
char rx_buffer[PAYLOAD];
int sock = 0;
char host_ip[] = HOST_IP_ADDR;
QueueHandle_t tcp_send_q = NULL;
rec_cb_t rec_cb = NULL;

//-----------------------------------------------------------------
static void tcp_receiver_task(void *pvParameters)
{
	while(true)
	{
		int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
		// Error occurred during receiving
		if (len < 0)
		{
				ESP_LOGE(TAG, "recv failed: errno %d", errno);
				break;
		}

		// Data received
		else
			if (rec_cb)
				rec_cb(rx_buffer, len);
	}
	if (sock != -1)
	{
			ESP_LOGE(TAG, "Shutting tcp_receiver_task");
			shutdown(sock, 0);
			close(sock);
	}
	vTaskDelete(NULL);
}

//-----------------------------------------------------------------
void tcp_client_task(void *pvParameters)
{
	int addr_family = 0;
	int ip_protocol = 0;
	tcp_send_q = xQueueCreate(8, sizeof(tcp_packet_t));	//for receive a message
	while (true)
	{
		struct sockaddr_in dest_addr;
		dest_addr.sin_addr.s_addr = inet_addr(host_ip);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;

		sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
		if (sock < 0)
		{
			ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
			break;
		}
		ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

		int err = 0;
		err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
		if (err)
		{
			ESP_LOGE(TAG, "%s: Socket unable to connect: err %d, errno %d", __func__, err, errno);
			shutdown(sock, 0);
			close(sock);
			continue;
		}
		ESP_LOGI(TAG, "Successfully connected");

		xTaskCreate(tcp_receiver_task, "tcp_receiver", 4096, NULL, 5, NULL);

		while (true)
		{
			tcp_packet_t pack;
			if(xQueueReceive(tcp_send_q, &pack, portMAX_DELAY))
			{
				int err = send(sock, pack.data, pack.size, 0);
				free (pack.data);
				if (err < 0)
				{
					ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
					break;
				}
			}
		}

		if (sock != -1)
		{
				ESP_LOGE(TAG, "Shutting down socket and restarting...");
				shutdown(sock, 0);
				close(sock);
		}
	}// while (true)
}

//-------------------------------------------------------------
esp_err_t msg_send_socket(void *data, uint32_t len)
{
	tcp_packet_t pack;
	pack.size = len;
	pack.data = NULL;
	pack.data = malloc(len);
	ESP_ERROR_CHECK(!pack.data);
	memcpy(pack.data, data, len);
	if (!tcp_send_q)
		return (ESP_ERR_INVALID_STATE);

	if(xQueueSend(tcp_send_q, &pack, 0) != pdPASS )
	{
		ESP_LOGW(TAG,"%s: outgoing message queue is full", __func__);
		return (ESP_ERR_NO_MEM);
	}
	return (ESP_OK);
}

//-------------------------------------------------------------
void tcp_receiver_cb_register(void *_rec_cb)
{
	rec_cb=_rec_cb;
}
