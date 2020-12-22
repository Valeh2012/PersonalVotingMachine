/**
 * @file WiFiModule.cpp
 * @brief WiFiModule implementation file
 * */

#include "string.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "module.h"

static const int WIFI_CONNECTED_BIT = BIT0;
static int MAXIMUM_RETRY = 5;
static int wifi_status = 0;

static int s_retry_num = 0;
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/** Sample code to connect wifi  from ESP-IDF*/
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < MAXIMUM_RETRY) {
			esp_wifi_connect();
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			s_retry_num++;
			ESP_LOGI("wi-fi", "retry to connect to the AP");
		}

		ESP_LOGI("wi-fi","connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		wifi_status = 1;
		ESP_LOGI("wi-fi","connected");
	}
}

WiFiModule::WiFiModule(char *ssid, char* pass){

	this->WIFI_SSID = ssid;
	this->WIFI_PASS = pass;
}

esp_err_t WiFiModule::init(){

	s_wifi_event_group = xEventGroupCreate();

	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_config_t wifi_config = {};
	memset(&wifi_config,0, sizeof(wifi_config));
	strcpy((char*)(wifi_config.sta.ssid), WIFI_SSID );
	strcpy((char*)(wifi_config.sta.password), WIFI_PASS);
	wifi_config.sta.bssid_set = false;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",WIFI_SSID, WIFI_PASS);

	while(!wifi_status){
		vTaskDelay(500 / portTICK_PERIOD_MS);
		if(s_retry_num >= MAXIMUM_RETRY){
			throw "WiFi unable to connect";
		}
	}
	return ESP_OK;
}

esp_err_t WiFiModule::deinit(){

	return esp_wifi_deinit();
}
