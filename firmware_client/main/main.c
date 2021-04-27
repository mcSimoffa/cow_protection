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

#include "tcp_sc.h"
#include "backup.h"
#include "wifi_connect.h"
#include "console.h"

//-----------------------------------------------------------------
#define WIFI_TIMEOUT_MS				CONFIG_WIFI_TIMEOUT_CONN
#define WIFI_TIMEOUT_TICK			(WIFI_TIMEOUT_MS/portTICK_PERIOD_MS)
//-----------------------------------------------------------------
static const char *TAG = "MAIN";

//-----------------------------------------------------------------
static void msg_rec_cb (void *data, uint32_t size)
{
	((char*)data)[size] = 0; // Null-terminate whatever we received and treat like a string
	ESP_LOGI(TAG, "%s Received %d bytes", __func__, size);
	ESP_LOGI(TAG, "%s", (char*)data);
}

//-----------------------------------------------------------------
void app_main(void)
{
	initialize_nvs();
	nvs_open_storage();
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	tcp_receiver_cb_register(msg_rec_cb);
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

	//XXX for test
	while (1)
	{
		char tst_str[]="Test string from ESP32 client";
		msg_send_socket((char*)tst_str, sizeof(tst_str)-1);
		vTaskDelay(100);
	}

}
