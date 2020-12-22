/**
 * @file SNTPModule.cpp
 * @brief SNTPModule implementation file
 * */

#include "lwip/apps/sntp.h"
#include "sntp/sntp.h"
#include "module.h"

void time_sync_notification_cb(struct timeval *tv){

	ESP_LOGI("Global","Notification of a time synchronization event");
}

/** Sample code to sync time with NTP server from ESP-IDF
 * https://github.com/espressif/esp-idf/tree/master/examples/protocols/sntp
 **/
void SNTPModule::initialize_sntp(void){

	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, (char*) "pool.ntp.org");
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
	sntp_init();
}

esp_err_t SNTPModule::init(){

	// Sync time
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	// Is time set? If not, tm_year will be (1970 - 1900).
	if (timeinfo.tm_year < (2019 - 1900)) {
		ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
		ESP_LOGI(TAG, "Initializing SNTP");
		initialize_sntp();
		int retry = 0;
		const int retry_count = 10;
		while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
			ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}
		// update 'now' variable with current time
		time(&now);
		localtime_r(&now, &timeinfo);
		char curr[20];
		strftime(curr, 20,"%Y-%m-%dT%XZ", localtime(&now));
		ESP_LOGI(TAG, "Time is set: %s", curr);
	}
	return ESP_OK;
}

esp_err_t SNTPModule::deinit(){

	sntp_stop();
	ESP_LOGI(TAG, "deinited");
	return ESP_OK;
}
