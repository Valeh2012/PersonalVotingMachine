/**
 *  OpenVotingClient project
 *
 */

/**
 *  @file main.cpp
 *  @brief Main application workflof is defined here
 *
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_freertos_hooks.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "Arduino.h"
#include "sdkconfig.h" // generated by "make menuconfig"

#include "lvgl/lvgl.h"
#include "module.h"
#include "controller.h"

// Define Wi-Fi parameters
#define WIFI_SSID      CONFIG_OVC_WIFI_SSID
#define WIFI_PASS      CONFIG_OVC_WIFI_PASSWORD

// Define server parameters
#define WEB_SERVER CONFIG_OVC_WEB_SERVER
#define DDS_SNI CONFIG_OVC_DDS_SNI
#define CHOICES_SNI CONFIG_OVC_CHOICES_SNI
#define VOTING_SNI CONFIG_OVC_VOTING_SNI
#define WEB_PORT CONFIG_OVC_WEB_PORT

// Define User Data
#define PHONE_NUMBER CONFIG_OVC_PHONE_NUMBER
#define ID_CODE CONFIG_OVC_ID_CODE

#define TAG "ivxv"

// Find your own device SDA and SCL pins (for OLED display)
#define SDA_PIN gpio_num_t( CONFIG_OVC_PIN_SDA )
#define SCL_PIN gpio_num_t( CONFIG_OVC_PIN_SCL )
#define RESET_PIN gpio_num_t( CONFIG_OVC_PIN_RESET )

// Reserve 3 pins for rotary encoder
gpio_num_t TOUCH_PIN_YP = gpio_num_t( CONFIG_OVC_PIN_TS_YP );	// Y+
gpio_num_t TOUCH_PIN_XP = gpio_num_t( CONFIG_OVC_PIN_TS_XP );	// X+
gpio_num_t TOUCH_PIN_XM = gpio_num_t( CONFIG_OVC_PIN_TS_XM );	// X-
gpio_num_t TOUCH_PIN_YM = gpio_num_t( CONFIG_OVC_PIN_TS_YM );	// Y-

/* Include external files ported to assembly binary */
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

extern const uint8_t server_public_key_pem_start[] asm("_binary_server_public_key_pem_start");
extern const uint8_t server_public_key_pem_end[]   asm("_binary_server_public_key_pem_end");

static void IRAM_ATTR lv_tick_task(void){

	lv_tick_inc((uint32_t)portTICK_RATE_MS);
}

extern "C" int app_main(){

	initArduino();

	ESP_ERROR_CHECK(gpio_install_isr_service(0));

	esp_register_freertos_tick_hook(lv_tick_task);

	try{

		ScreenModule screen = ScreenModule::Instance();
		screen.init();

		TouchModule tm = TouchModule(TOUCH_PIN_YP, TOUCH_PIN_XP, TOUCH_PIN_XM, TOUCH_PIN_YM);
		tm.init();

		IndexController indxCtrl = IndexController();
		indxCtrl.index();

		WiFiModule wifi = WiFiModule(WIFI_SSID, WIFI_PASS);
		wifi.init();

		SNTPModule sntp = SNTPModule();
		sntp.init();

		RPC::Instance().cfg->cacert_pem_buf  = server_root_cert_pem_start,
		RPC::Instance().cfg->cacert_pem_bytes = server_root_cert_pem_end - server_root_cert_pem_start;
		RPC::Instance().server = (char *) WEB_SERVER;
		RPC::Instance().port = (int) WEB_PORT;

		UserModel um = UserModel((char *)ID_CODE,(char *) PHONE_NUMBER);
		AuthorizationController authCtrl = AuthorizationController(&um, DDS_SNI);

		authCtrl.index();
		authCtrl.auth();
		authCtrl.authStatus();

		ChoiceModel cm = ChoiceModel(um.ssid, um.authToken);
		ChoiceController choiceCtrl = ChoiceController(&cm, CHOICES_SNI);

		choiceCtrl.index();

		EncryptionModel em = EncryptionModel(cm.ballot, cm.ssid, cm.authToken, server_public_key_pem_start, server_public_key_pem_end - server_public_key_pem_start);
		EncryptionController encCtrl = EncryptionController(&em);

		encCtrl.index();

		SignatureModel sm = SignatureModel(em.ssid, em.authToken, um.phone, um.ID, em.ballotHash, em.ballotLength, em.ballotFileName);
		SignatureController signCtrl = SignatureController(&sm, DDS_SNI);

		signCtrl.index();
		signCtrl.sign();
		signCtrl.status();
		signCtrl.combine();

		ZipModel zm = ZipModel(em.ballotASN, em.ballotFileName, em.ballotLength, sm.Signature);
		ZipController zipCtrl = ZipController(&zm);

		zipCtrl.index();

		VoteModel vm = VoteModel(cm.ssid, cm.authToken, zm.voteBase64, cm.choices);
		VoteController voteCtrl = VoteController(&vm, VOTING_SNI);

		voteCtrl.index();

		QRModel qm = QRModel(vm.ssid, vm.voteID, em.rndBase64);
		QRController qrCtrl = QRController(&qm);

		qrCtrl.index();

		indxCtrl.end();
	}
	catch(const char* msg){
		puts(msg);
	}

	fflush(stdout);
	return 0;
}
