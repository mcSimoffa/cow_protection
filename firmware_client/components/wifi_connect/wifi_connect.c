#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "backup.h"
#include "wifi_connect.h"

//******************************************  DEFINES  ********************************

#define DEFAULT_SCAN_LIST_SIZE 	10	// maximum 255
#define FAMOUS_SSIDS_QTT				12
//----------------------
#define SIZEOF_FAMOUS_SSIDS			(FAMOUS_SSIDS_QTT + 1)
#define KEY_WIFI_CACHE_NVS			"WIFI_CONN_CACHE"
#define WIFI_CONNECTED_BIT 	BIT0
#define WIFI_FAIL_BIT      	BIT1
#define TIMEOUT_CONN_MS			CONFIG_WIFI_TIMEOUT_CONN
#define TIMEOUT_CONN_TICK		(TIMEOUT_CONN_MS/portTICK_PERIOD_MS)

//*************************************** GLOBAL VARIABLES ****************************
typedef struct
{
	char 		psw[64];
	char 		ssid[SSID_LEN+1];
	bool		enable;
} ap_data_t;

SemaphoreHandle_t xmut_wifiEvt;

static EventGroupHandle_t s_wifi_event_group = NULL; //FreeRTOS event group to signal when we are connected
static const char *TAG = "WiFiCONN";

ap_data_t priority_connect_table[SIZEOF_FAMOUS_SSIDS] = {0};

//*********************************** FUNCTION DEFINITIONS ****************************

static void vizualize_priority_table()
{
#if defined (CONFIG_LOG_DEFAULT_LEVEL_INFO) || defined (CONFIG_LOG_DEFAULT_LEVEL_DEBUG) || defined (CONFIG_LOG_DEFAULT_LEVEL_VERBOSE)
	for (int8_t j=0; j < SIZEOF_FAMOUS_SSIDS; j++)
		ESP_LOGI(TAG, "%s: famous_ssid[%d] SSID %s \tpassword %s.", __func__, j, priority_connect_table[j].ssid, priority_connect_table[j].psw);
#endif
}


//------------------------------------------------------------------
static void priority_table_rearrange(uint8_t recomend_elem_num)
{
	if (recomend_elem_num == 0) return;
	ap_data_t temp;
	memcpy(&temp, &priority_connect_table[recomend_elem_num], sizeof(ap_data_t));
	memmove(&priority_connect_table[1], &priority_connect_table[0], sizeof(ap_data_t) * recomend_elem_num);
	memcpy(&priority_connect_table[0], &temp, sizeof(ap_data_t));
	vizualize_priority_table();
}

//----------------------------------------------------------------------------------------------------------------------------------
static void wifi_ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	static uint16_t number = DEFAULT_SCAN_LIST_SIZE;
	static wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
	static uint16_t ap_count;
	static uint8_t begin_index;
	static bool connectingFlag = false;

	xSemaphoreTake(xmut_wifiEvt, portMAX_DELAY);
	if (event_base == WIFI_EVENT)
	{
		switch(event_id)
		{
			case WIFI_EVENT_STA_START:
				ESP_LOGI(TAG, "%s: Scan start", __func__);
				ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
				break;

			case WIFI_EVENT_STA_DISCONNECTED:
				ESP_LOGI(TAG, "%s: Scan disconnect", __func__);
				connectingFlag = false;
				xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
				xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
				goto l_try_connect;

			case WIFI_EVENT_SCAN_DONE:
				ESP_LOGI(TAG, "%s: EVENT_SCAN_DONE", __func__);
				number = DEFAULT_SCAN_LIST_SIZE;
				memset(ap_info, 0, sizeof(ap_info));
				ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
				ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
				/*ESP_LOGI(TAG, "%s: Total APs scanned = %d or %d", __func__, ap_count, number);
				for (int8_t i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++)
					ESP_LOGI(TAG, "%s: SSID %s \tRSSI %d", __func__, ap_info[i].ssid, ap_info[i].rssi);*/
				begin_index = 0;
				connectingFlag = false;

l_try_connect:
				for (int8_t j=begin_index; j < SIZEOF_FAMOUS_SSIDS; j++)
				{
					if (!priority_connect_table[j].enable)	continue;
					for (int8_t i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++)
					{
						if (strcmp((char*)ap_info[i].ssid, priority_connect_table[j].ssid) == 0)
						{
							ESP_LOGI(TAG, "%s: Try connect to SSID %s \t(RSSI %d) \tpassword %s.", __func__, ap_info[i].ssid, ap_info[i].rssi, priority_connect_table[j].psw);
							wifi_config_t wifi_config ={0};
							strcpy ((char*)&wifi_config.sta.ssid, 		priority_connect_table[j].ssid);
							strcpy ((char*)&wifi_config.sta.password, priority_connect_table[j].psw);
							wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;
							wifi_config.sta.pmf_cfg.capable = true;
							wifi_config.sta.pmf_cfg.required = false;
							ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
							begin_index = j + 1;
							connectingFlag = true;
							esp_wifi_connect();
							break;
						}
					}
					if (connectingFlag) break;
				}

				if (connectingFlag == false)
				{
					ESP_LOGI(TAG, "%s: Don't have available SSIDs Scan restart", __func__);
					ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
				}
				break;

			case WIFI_EVENT_STA_CONNECTED:
				ESP_LOGI(TAG, "%s: EVENT_STA_CONNECTED at index %d", __func__, begin_index - 1);
				priority_table_rearrange(begin_index - 1);
				begin_index = 0;
				//set autoconnect mode
				for (int8_t j=0; j<FAMOUS_SSIDS_QTT; j++)
						priority_connect_table[j].enable = true;
				priority_connect_table[FAMOUS_SSIDS_QTT].enable = false;
				break;
		}
	} else 	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
					{
						ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
						ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
						xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
						xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

					}
	xSemaphoreGive(xmut_wifiEvt);
}

//--------------------------------------------------------------------------
esp_err_t wifi_connect()
{
	static esp_netif_t*  def_wifista = NULL;
	esp_log_level_set("wifi", ESP_LOG_WARN);
	if (def_wifista != NULL)
		return (ESP_ERR_INVALID_STATE);

	def_wifista = esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ip_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_ip_event_handler, NULL));
	xmut_wifiEvt = xSemaphoreCreateMutex();
	ESP_ERROR_CHECK(xmut_wifiEvt == NULL);
	if (s_wifi_event_group == NULL)
			s_wifi_event_group = xEventGroupCreate();
	xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
	ESP_ERROR_CHECK(esp_wifi_start());

//wait nitification about incoming BLE packet with connection parameters
	//while (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 1)
	//	ESP_LOGI(TAG, "%s: Wait from BLE ...", __func__);

	// Waiting until either the connection is established or timeout expiered
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
					WIFI_CONNECTED_BIT,
					pdFALSE,	//not clear after waiting
					pdFALSE,	//return if any bit will be '1'
					TIMEOUT_CONN_TICK);

	return ((bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_ERR_TIMEOUT);
}

//-------------------------------------------------------------------------
bool get_wifi_state(TickType_t xTicksToWait)
{
	if (s_wifi_event_group)
	{
		EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
								WIFI_CONNECTED_BIT,
								pdFALSE,	//not clear after waiting
								pdFALSE,	//return if any bit will be '1'
								xTicksToWait);

		return ((bits & WIFI_CONNECTED_BIT) ? true : false);
	}
	return (false);
}


//--------------------------------------------------------------------------
void wifi_power_safe(bool action)
{
	wifi_ps_type_t psmode;
	ESP_ERROR_CHECK(esp_wifi_get_ps(&psmode));
	if ((action) && (psmode != WIFI_PS_MIN_MODEM))
	{
		psmode = WIFI_PS_MIN_MODEM;
		ESP_ERROR_CHECK(esp_wifi_set_ps(psmode));
	}
	else if ((!action) && (psmode != WIFI_PS_NONE))
				{
					psmode = WIFI_PS_NONE;
					ESP_ERROR_CHECK(esp_wifi_set_ps(psmode));
				}
	ESP_LOGI(TAG, "%s: WiFi power safe mode %d", __func__, psmode);
}

//------------------------------------------------------------------
void wifi_sta_manual_connect(char *ssid, char *psw)
{
	if ((!ssid) || (*ssid == 0))
	{
		wifi_set_mode_auto_connect();
		return;
	}

	xSemaphoreTake(xmut_wifiEvt, portMAX_DELAY);

	for (int8_t j=0; j<FAMOUS_SSIDS_QTT; j++)
			priority_connect_table[j].enable = false;

	// trying find this 'ssid' in exiting table
	for (int8_t j=0; j<FAMOUS_SSIDS_QTT; j++)
	{
		if (strcmp(ssid, priority_connect_table[j].ssid) == 0)
		{
			strcpy (priority_connect_table[j].psw, psw);
			priority_connect_table[j].enable = true;
			goto exit_prepare;
		}
	}
	//if this 'ssid' isn't exist - then adding it at last place
	strcpy(priority_connect_table[FAMOUS_SSIDS_QTT].ssid, ssid);
	strcpy(priority_connect_table[FAMOUS_SSIDS_QTT].psw, psw);
	priority_connect_table[FAMOUS_SSIDS_QTT].enable = true;

exit_prepare:
	if(get_wifi_state(0))		esp_wifi_disconnect();
	xSemaphoreGive(xmut_wifiEvt);
}


//------------------------------------------------------------------
void wifi_set_mode_auto_connect()
{
	xSemaphoreTake(xmut_wifiEvt, portMAX_DELAY);
	for (int8_t j=0; j<FAMOUS_SSIDS_QTT; j++)
		priority_connect_table[j].enable = true;

	priority_connect_table[FAMOUS_SSIDS_QTT].enable = false;
	xSemaphoreGive(xmut_wifiEvt);
}

//------------------------------------------------------------------
void saveNVS_priority_table()
{
	if (write_nvs_array(KEY_WIFI_CACHE_NVS, &priority_connect_table, sizeof(priority_connect_table)) == ESP_OK)
		ESP_LOGD(TAG, "%s: store %d bytes to NVS", __func__, sizeof(priority_connect_table));
	else
		ESP_LOGE(TAG, "%s: Error store %d bytes to NVS", __func__, sizeof(priority_connect_table));
}


//------------------------------------------------------------------
esp_err_t wifi_git_ap_ssid(char *p_ssid)
{
	if (get_wifi_state(0))
	{
		wifi_ap_record_t ap_info;
		if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
		{
			strncpy(p_ssid, (char*)ap_info.ssid, SSID_LEN);
			return (ESP_OK);
		}
	}
	return (ESP_FAIL);
}


//------------------------------------------------------------------
void retreive_priority_table()
{
	void *p_arr = NULL;
	size_t  size_arr;
	if (read_nvs_array(KEY_WIFI_CACHE_NVS, &p_arr, &size_arr) == ESP_OK)
	{
		ESP_LOGD(TAG, "%s: read %d bytes from NVS", __func__, size_arr);
		uint32_t elem = size_arr/sizeof(ap_data_t);
		if (size_arr <= sizeof(priority_connect_table))
		{
			memcpy(&priority_connect_table, p_arr, size_arr);
			ESP_LOGI(TAG, "%s: read %d elements from NVS", __func__, elem);
		}
		free (p_arr);
	}
	vizualize_priority_table();
}
