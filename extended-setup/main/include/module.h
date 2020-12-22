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
	gpio_num_t RESET_PIN = GPIO_NUM_NC; /**< OLED RESET connection pin number  */

	void setSCL(gpio_num_t scl_pin); /**< set SCL pin  */
	void setSDA(gpio_num_t sda_pin); /**< set SCL pin  */
	void setRST(gpio_num_t rst_pin); /**< set RESET pin  */
	esp_err_t init();
	esp_err_t deinit();
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
 * \class TouchModule
 * \brief TouchScreen module
 * */
class TouchModule : public Module {
private:
	const char* TAG = "Touch";
	gpio_num_t YP; /**< Y+ pin. Should connect to only Analog pin */
	gpio_num_t YM; /**< Y- pin. Should connect to any digital pin */
	gpio_num_t XP; /**< X+ pin. Should connect to any digital pin */
	gpio_num_t XM; /**< X- pin. Should connect to only Analog pin */
public:
	TouchModule(gpio_num_t a, gpio_num_t b, gpio_num_t c, gpio_num_t d);
	esp_err_t init();
	esp_err_t deinit();
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

#endif /* MAIN_INCLUDE_MODULE_H_ */
