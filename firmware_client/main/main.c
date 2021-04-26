#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"
//#include "addr_from_stdin.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

#include "backup.h"
#include "wifi_connect.h"
#include "console.h"

//-----------------------------------------------------------------
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#define PORT CONFIG_EXAMPLE_PORT
#define PARAMETERS_IN_RAM_QTT	32

#define WIFI_TIMEOUT_MS				CONFIG_WIFI_TIMEOUT_CONN
#define WIFI_TIMEOUT_TICK			(WIFI_TIMEOUT_MS/portTICK_PERIOD_MS)
//-----------------------------------------------------------------
static const char *TAG = "example";
static const char *payload = "Message from ESP32 ";
char rx_buffer[128];
int sock = 0;
char host_ip[] = HOST_IP_ADDR;

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
		{
			rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
			ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
			ESP_LOGI(TAG, "%s", rx_buffer);
		}
	}
	if (sock != -1)
	{
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			shutdown(sock, 0);
			close(sock);
	}
	vTaskDelete(NULL);
}

//-----------------------------------------------------------------
static void tcp_client_task(void *pvParameters)
{
	int addr_family = 0;
	int ip_protocol = 0;

	while (1)
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

		int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
		if (err != 0)
		{
					ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
					break;
		}
		ESP_LOGI(TAG, "Successfully connected");

		xTaskCreate(tcp_receiver_task, "tcp_receiver", 4096, NULL, 5, NULL);

		while (1)
		{
			int err = send(sock, payload, strlen(payload), 0);
			if (err < 0)
			{
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
				break;
			}

			vTaskDelay(300 / portTICK_PERIOD_MS);
		}

		if (sock != -1)
		{
				ESP_LOGE(TAG, "Shutting down socket and restarting...");
				shutdown(sock, 0);
				close(sock);
		}
	}
	vTaskDelete(NULL);
}

//-----------------------------------------------------------------
void app_main(void)
{
	initialize_nvs();
	nvs_open_storage();
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	xTaskCreate(consoleProcessor_task, "console", 4096, NULL, 2, NULL);

	// wifi connect
	retreive_priority_table();
	if (wifi_connect () == ESP_ERR_TIMEOUT)
	{
		 do
			 ESP_LOGW(TAG, "%s: WiFi connection failed", __func__);
		 while (!get_wifi_state(WIFI_TIMEOUT_TICK));
	}
	saveNVS_priority_table();

	// TCP socket
	xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);

}
