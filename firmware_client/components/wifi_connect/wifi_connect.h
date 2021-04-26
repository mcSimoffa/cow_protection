
#ifndef _WIFI_CONNECT_H_
#define _WIFI_CONNECT_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"


#define SSID_LEN							32

/* *******************************************************************
Initiate Wi-Fi connection.
Function flow:
1. Retrieve from NVS a table with stored previous connection data
(SSID and Password)
2. Scan network and get a list of access points
3. Trying connect with one of last time connected AP if it exist
4. In case success connected  it set this SSID to a priority place
5. In case AP disapear a station will try connect to another AP from
priority table automatically

 !! ATTENTION !!
 application must store to NVS a priority table by saveNVS_priority_table()

  return 	ESP_OK - if success cWiFi connect
  				ESP_ERR_TIMEOUT - if timeout connection is occured
  				ESP_ERR_INVALID_STATE - if wi-fi connect is already launch
 ****************************************************************** */
esp_err_t wifi_connect();


/* *******************************************************************
  Get status of WiFi connect
	param [in]	xTicksToWait - time for wait connection state
							if 0 the non blocking mode.

  return 	true 	if WiFi connect
 * 				false if WiFi not connect
 ****************************************************************** */
bool get_wifi_state(TickType_t xTicksToWait);

/* *******************************************************************
  Function directly use their ssid and psw and initiate connection
  if success then this ssid and password will be store in priority table
  A connection status can to know by  get_wifi_state();
  param [in]	ssid - pointer to ssid string (32 char max)
  	if 'ssid' = NULL or empty string - there will be set
  	automatical mode connect by launch wifi_set_mode_auto_connect()
  param [in]	psw - pointer to password string (63 char max)
 ****************************************************************** */
void wifi_sta_manual_connect(char *ssid, char *psw);


/* *******************************************************************
  Function for switch mode to automatic connect
  in this mode station will try connect to AP from priority table
 ****************************************************************** */
void wifi_set_mode_auto_connect();


/* *******************************************************************
  Function for contron power safe mode
  param [in]	action - 0 - WIFI_PS_NONE
  										 1 - WIFI_PS_MIN_MODEM
 ****************************************************************** */
void wifi_power_safe(bool action);


/* *******************************************************************
  Save priority table to NVS.
  This function should be launch before poer off procedure
 ****************************************************************** */
void saveNVS_priority_table();


/* *******************************************************************
  Function return a access point ssid which connected it for
  param [out]	p_ssid - pointer to char[33]={0} array where will be store
  	'ssid' string
  return 	ESP_OK - if success
  				ESP_FAIL - if WiFi disconnect or another errors
 ****************************************************************** */
esp_err_t wifi_git_ap_ssid(char *p_ssid);


/* ********************************************************
Function for get from NVS to RAM location array 'ssid' and passwords
 * ****************************************************** */
void retreive_priority_table();

#endif
