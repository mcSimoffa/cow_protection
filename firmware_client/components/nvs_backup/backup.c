#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "backup.h"

//******************************************  DEFINES  ********************************
#define STORAGE_NAMESPACE	"PARAM_BACKUP"


//*************************************** GLOBAL VARIABLES ****************************
static const char* TAG = "BACKUP";
nvs_handle_t backup_h = 0;


//*********************************** FUNCTION DEFINITIONS ****************************

//---------------------------------------------------------------------------------------------
void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

//-----------------------------------------------------------
void nvs_open_storage()
{
	 esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &backup_h);
	if (err != ESP_OK)
		ESP_LOGE(TAG, "%s NVC open error %d (%s)", __func__, err, esp_err_to_name(err));
	ESP_ERROR_CHECK(err);
}

//-----------------------------------------------------------
esp_err_t read_nvs_integer(const char* key, int *pDst)
{
	esp_err_t err = nvs_get_i32(backup_h, key, pDst);

	if (err != ESP_OK)
		ESP_LOGW(TAG, "%s: NVC read error %d (%s)", __func__, err, esp_err_to_name(err));
	return (err);
}

//-----------------------------------------------------------
esp_err_t read_nvs_str(const char* key, char **ppDst)
{
	size_t required_size;
	*ppDst = NULL;
	esp_err_t err = nvs_get_str(backup_h, key, NULL, &required_size);
	if (err == ESP_OK)
	{
		*ppDst = malloc(required_size);
		if (*ppDst != NULL)
			err = nvs_get_str(backup_h, key, *ppDst, &required_size);
		else
			{
				ESP_LOGW(TAG, "%s NVC malloc out of memory: %d bytes", __func__, required_size);
				err = ESP_ERR_NO_MEM;
			}
	}
	if (err != ESP_OK)
		ESP_LOGW(TAG, "%s: NVC read error %d (%s)", __func__, err, esp_err_to_name(err));
	return (err);
}


//-----------------------------------------------------------
esp_err_t read_nvs_array(const char* key, void **ppDst, size_t *size)
{
	*ppDst = NULL;
	esp_err_t err = nvs_get_blob(backup_h, key, NULL, size);
	if (err == ESP_OK)
	{
		*ppDst = malloc(*size);
		if (*ppDst != NULL)
			err = nvs_get_blob(backup_h, key, *ppDst, size);
		else
			{
				ESP_LOGW(TAG, "%s NVC malloc out of memory: %d bytes", __func__, *size);
				err = ESP_ERR_NO_MEM;
			}
	}
	if (err != ESP_OK)
		ESP_LOGW(TAG, "%s: NVC read error %d (%s)", __func__, err, esp_err_to_name(err));
	return (err);
}

//-----------------------------------------------------------
esp_err_t write_nvs_integer(const char* key, int toSave)
{
	esp_err_t err = nvs_set_i32 (backup_h, key, toSave);
	if (err == ESP_OK)
	  err = nvs_commit(backup_h);

	if (err != ESP_OK)
		ESP_LOGE(TAG, "%s: NVC write error %d (%s)", __func__, err, esp_err_to_name(err));
	return (err);
}

//-----------------------------------------------------------
esp_err_t write_nvs_str(const char* key, char *pSrc)
{
	esp_err_t err = nvs_set_str(backup_h, key, pSrc);
	if (err == ESP_OK)
	  err = nvs_commit(backup_h);

	if (err != ESP_OK)
		ESP_LOGE(TAG, "%s: NVC write error %d (%s)", __func__, err, esp_err_to_name(err));
	return (err);
}

//-----------------------------------------------------------
esp_err_t write_nvs_array(const char* key, void *pSrc, size_t size)
{
	esp_err_t err = nvs_set_blob(backup_h, key, pSrc, size);
	if (err == ESP_OK)
	  err = nvs_commit(backup_h);

	if (err != ESP_OK)
		ESP_LOGE(TAG, "%s: NVC write error %d (%s)", __func__, err, esp_err_to_name(err));
	return (err);
}

//-----------------------------------------------------------
esp_err_t erase_nvs_record(const char* key)
{
	esp_err_t err =nvs_erase_key(backup_h, key);
	if (err == ESP_OK)
		  err = nvs_commit(backup_h);

	if (err != ESP_OK)
		ESP_LOGE(TAG, "%s: NVC erase error %d (%s)", __func__, err, esp_err_to_name(err));
	return (err);
}

