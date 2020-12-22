#ifndef MAIN_INCLUDE_MODULE_H_
#define MAIN_INCLUDE_MODULE_H_

/**
 * @file module.h
 * @brief Header for module definitions
 * */

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "cJSON.h"
#include "esp_gap_ble_api.h"

#include "rotary_encoder.h"

/**
 * \class Module
 * \brief base abstract module class
 * */
class Module{
public:

	/**
	 * @brief module initialization abstract method
	 * */
	virtual esp_err_t init()=0;

	/**
	* @brief module deinitialization abstract method
	* */
	virtual esp_err_t deinit()=0;

	/**
	 * @brief default module destructor
	 * */
	virtual ~Module(){};
};

/**
 * \class RPC
 * \brief remote procedure call module
 *
 * This module follows singleton pattern to prevent concurrent tls communications which may abort program
 * */
class RPC{
private:
	const char* TLS_TAG = "rpc";

	RPC();
public:
	esp_tls_cfg_t *cfg;  /**< tls connection config file */
	char* server;  /**< host server address (full domain or ipaddress) */
	int port; /**< host server process port, default 443 */

	/**
	 * @brief RPC Communication method.
	 *
	 *
	 * @param const char* sni SNI of the service to make a request
	 * @param const char* req stringified JSON request body
	 *
	 * @returns pointer to cJSON structure variable that holds parsed JSON response
	 * */
	cJSON * send_json_rpc(const char* sni, char* req);

public:
	static RPC& Instance();  /**< singleton RPC instance*/
};

/**
 * \class ScreenModule
 * \brief screen device module
 *
 * This module follows singleton pattern to prevent unexpected behaviors when two process concurrently want to write data
 * */
class ScreenModule : public Module{
private:
	const char* TAG = "ssd_1306";
	uint8_t screen_id = 0;  /**< connected screen id */
	uint8_t lineOff = 0; /**< last written line  */

	ScreenModule();
public:
	gpio_num_t SCL_PIN; /**< SCL connection pin number  */
	gpio_num_t SDA_PIN; /**< SDA connection pin number  */
	gpio_num_t RESET_PIN; /**< RESET connection pin number  */

	void setSCL(gpio_num_t scl_pin); /**< set SCL pin  */
	void setSDA(gpio_num_t sda_pin); /**< set SCL pin  */
	void setRST(gpio_num_t rst_pin); /**< set RST pin  */
	esp_err_t init();
	esp_err_t deinit();

	/* UI method to write strings to screen */
	void print_screen(char *buff); /**< print char buffer to screen */
	void clear();  /**< clear screen */
	void refresh(); /**< refresh screen to update content */

	/** @brief write one line to screen
	 *
	 * @param str char buffer to write
	 * @param pl  left padding
	 * */
	void writeLine(char *str, uint8_t pl);

	/** @brief draw white circle on screen
	 *
	 * @param x x coordinate of circle center
	 * @param y y coordinate of circle center
	 * @param r circle radius
	 * */
	void FillCircle(uint8_t x, uint8_t y, uint8_t r);
public:
	static ScreenModule& Instance(); /**< singleton ScreenModule instance*/
};

/**
 * \class WiFiModule
 * \brief WiFi connections module
 * */
class WiFiModule : public Module{
private:
	const char * TAG = "wi-fi";
	char* WIFI_SSID; /**< WiFi SSID name */
	char* WIFI_PASS; /**< WiFi password */
public:
	WiFiModule(char *ssid, char* pass);
	esp_err_t init();
	esp_err_t deinit();
};

/**
 * \class NVSModule
 * \brief NVS module for board initialization
 * */
class NVSModule : public Module{
private:
	const char* TAG = "nvs";
public:
	esp_err_t init();
	esp_err_t deinit();
};

/**
 * \class RotaryEncoderModule
 * \brief RotaryEncoder module to control attached rotary encoder device. Listens to rotations and click events.
 * */
class RotaryEncoderModule : public Module{
private:
	const char* TAG = "RotEnc";
	RotaryEncoderModule();
public:
	gpio_num_t ROT_ENC_A_GPIO, ROT_ENC_B_GPIO, ROT_ENC_C_GPIO;
	bool ENABLE_HALF_STEPS;
	bool FLIP_DIRECTION;
	rotary_encoder_info_t *info;

	/** @brief set gpio pins connected to rotary encoder
	 *
	 * @param a pin connected to DT bus
	 * @param b pin connected to CLK bus
	 * @param c pin connected to SW bus
	 * */
	void setPins(gpio_num_t a, gpio_num_t b, gpio_num_t c);

	rotary_encoder_info_t* getInfo() const;
	esp_err_t init();
	esp_err_t deinit();
	static RotaryEncoderModule& Instance(); /**< singleton RotaryEncoderModule instance */
	RotaryEncoderModule(RotaryEncoderModule const&) = delete;  /**< Delete public constructor */
	void operator=(RotaryEncoderModule const&)      = delete;  /**< Disallow object copying */
};

/**
 * \class SNTPModule
 * \brief Time synchronization module
 * */
class SNTPModule : public Module{
private:
	const char* TAG = "sntp";
public:
	static void initialize_sntp(void); /**< synchronize time over network*/
	esp_err_t init();
	esp_err_t deinit();
};

/**
 * \class BLEModule
 * \brief BLE communication module
 * */
class BLEModule : public Module{
public:
	const char* TAG = "ble";

	esp_err_t init();
	esp_err_t deinit();
	BLEModule();
	esp_ble_adv_params_t * adv; /**< bluetooth device advertisement params pointer*/
};

#endif /* MAIN_INCLUDE_MODULE_H_ */
