/**
 * @file NVSModule.cpp
 * @brief NVSModule implementation file
 * */

#include "nvs_flash.h"
#include "module.h"

esp_err_t NVSModule::init(){

	ESP_LOGI(TAG, "initializing nvs");
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();
	}
	return ret;
}

esp_err_t NVSModule::deinit(){

	return nvs_flash_deinit();
}
