/**
 * @file ScreenModule.cpp
 * @brief ScreenModule implementation file
 * */

#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include "module.h"

u8g2_t u8g2; // a structure which will contain all the data for one display

ScreenModule::ScreenModule(){};

void ScreenModule::setSCL(gpio_num_t scl_pin){

	this->SCL_PIN = scl_pin;
}

void ScreenModule::setSDA(gpio_num_t sda_pin){

	this->SDA_PIN = sda_pin;
}

void ScreenModule::setRST(gpio_num_t rst_pin){

	this->RESET_PIN = rst_pin;
}

esp_err_t ScreenModule::init(){

	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.sda   = SDA_PIN;
	u8g2_esp32_hal.scl  = SCL_PIN;

	if(RESET_PIN != GPIO_NUM_NC) u8g2_esp32_hal.reset  = RESET_PIN;
	u8g2_esp32_hal_init( u8g2_esp32_hal );

	u8g2_Setup_ssd1306_i2c_128x64_noname_f(
		&u8g2,
		U8G2_R0,
		//u8x8_byte_sw_i2c,
		u8g2_esp32_i2c_byte_cb,
		u8g2_esp32_gpio_and_delay_cb);  // init u8g2 structure
	try{
		u8x8_SetI2CAddress(&u8g2.u8x8,0x78);
		ESP_LOGI(TAG, "u8g2_InitDisplay");
		u8g2_InitDisplay(&u8g2);
		ESP_LOGI(TAG, "u8g2_SetPowerSave");
		u8g2_SetPowerSave(&u8g2, 0); // wake up display
		ESP_LOGI(TAG, "u8g2_ClearBuffer");
		u8g2_ClearBuffer(&u8g2);
	}catch(const char* msg){
		throw msg;
	}
	return ESP_OK;
}

esp_err_t ScreenModule::deinit(){

	u8g2_ClearBuffer(&u8g2);
	return ESP_OK;
}

/* UI method to write strings to screen */
void ScreenModule::print_screen(char *buff){

	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_Georgia7px_tf);
	u8g2_DrawStr(&u8g2, 0,10, buff);
	u8g2_SendBuffer(&u8g2);
}

void ScreenModule::clear(){

	u8g2_ClearBuffer(&u8g2);
	lineOff = 0;
}

void ScreenModule::refresh(){

	u8g2_SendBuffer(&u8g2);
}


void ScreenModule::writeLine(char *str, uint8_t pl){

	u8g2_SetFont(&u8g2, u8g2_font_4x6_tf);
	u8g2_DrawStr(&u8g2, pl, lineOff*10+10, str);
	lineOff++;
}

void ScreenModule::FillCircle(uint8_t x, uint8_t y, uint8_t r){
	u8g2_DrawDisc(&u8g2, x, y, r, U8G2_DRAW_ALL);
}

ScreenModule& ScreenModule::Instance(){

	static ScreenModule instance;
	return instance;
}
