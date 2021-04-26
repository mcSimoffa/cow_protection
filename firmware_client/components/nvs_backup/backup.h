#ifndef _BACKUP_H_
#define _BACKUP_H_


#include "esp_err.h"
#include "nvs_flash.h"

/* ********************************************************
Function for initialize Non volatile storage NVS
 * ****************************************************** */
void initialize_nvs(void);


/* ********************************************************
Function for open Non volatile storage NVS
it use 'STORAGE_NAMESPACE' define in backup.c
 * ****************************************************** */
void nvs_open_storage();


/* ********************************************************
Function for read a integer value from a flash storage
param [in] key - identificator, key for this variable
	  [out] pDst - pointer to the place for store a variable

return: ESP_OK if successful read
 	 	 otherwise if any wrong
 * ****************************************************** */
esp_err_t read_nvs_integer(const char* key, int *pDst);



/* ********************************************************
Function for read a string value from a flash storage
param [in] key - identificator, key for this variable
	  	[out] ppDst - pointer to pointer the at string begin
!!!!! ATTENTION !!!!
inside this function there are allocation of memory for string
You must free() this block like: free (*ppDst)

 return: ESP_OK if successful read
 	 	 otherwise if any wrong
 * ****************************************************** */
esp_err_t read_nvs_str(const char* key, char **ppDst);


/* ********************************************************
Function for read an array from a flash storage
param [in] key - identificator, key for this variable
	  	[out] ppDst - pointer to pointer the at array begin
	  	[out] size  - pointer to returned size variable
!!!!! ATTENTION !!!!
inside this function there are allocation of memory for string
You must free() this block like: free (*ppDst)

 return: ESP_OK if successful read
 	 	 otherwise if any wrong
 * ****************************************************** */
esp_err_t read_nvs_array(const char* key, void **ppDst, size_t *size);

/* ********************************************************
Function for write a integer value to the flash storage
param [in] key - identificator, key for this variable
	  	[in] toSave - stored value

return: ESP_OK if successful read
 	 	 otherwise if any wrong
 * ****************************************************** */
esp_err_t write_nvs_integer(const char* key, int toSave);



/* ********************************************************
Function for write a string to the flash storage
param [in] key - identificator, key for this variable
	  [in] pSrc - pointer to the string begin

return: ESP_OK if successful read
 	 	 otherwise if any wrong
 * ****************************************************** */
esp_err_t write_nvs_str(const char* key, char *pSrc);



/* ********************************************************
Function for write an array to the flash storage
param [in] key - identificator, key for this variable
	  	[in] pSrc - pointer to the array begin
	  	[in] size - size of array

return: ESP_OK if successful read
 	 	 otherwise if any wrong
 * ****************************************************** */
esp_err_t write_nvs_array(const char* key, void *pSrc, size_t size);


/* ********************************************************
Function for erase a pair key:value
param [in] key - identificator, key for this variable

return: ESP_OK if successful erase
 	 	 otherwise if any wrong
 * ****************************************************** */
esp_err_t erase_nvs_record(const char* key);

#endif //_BACKUP_H_
