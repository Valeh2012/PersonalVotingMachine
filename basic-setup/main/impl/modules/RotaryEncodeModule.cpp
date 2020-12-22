/**
 * @file RotaryEncodeModule.cpp
 * @brief RotaryEncoderModule implementation file
 * */

#include "module.h"

RotaryEncoderModule::RotaryEncoderModule(){};

void RotaryEncoderModule::setPins(gpio_num_t a, gpio_num_t b, gpio_num_t c){

	ROT_ENC_A_GPIO = a;
	ROT_ENC_B_GPIO = b;
	ROT_ENC_C_GPIO = c;
	ENABLE_HALF_STEPS = false;
	FLIP_DIRECTION = false;
	this->info = new rotary_encoder_info_t();
}

rotary_encoder_info_t* RotaryEncoderModule::getInfo() const{

	return info;
}

esp_err_t RotaryEncoderModule::init(){

	// Initialise the rotary encoder device with the GPIOs for A and B signals
	ESP_ERROR_CHECK(rotary_encoder_init(info, ROT_ENC_A_GPIO, ROT_ENC_B_GPIO, ROT_ENC_C_GPIO));
	ESP_ERROR_CHECK(rotary_encoder_enable_half_steps(info, ENABLE_HALF_STEPS));
	if( FLIP_DIRECTION )
		ESP_ERROR_CHECK(rotary_encoder_flip_direction(info));

	// Create a queue for events from the rotary encoder driver.
	// Tasks can read from this queue to receive up to date position information.
	QueueHandle_t event_queue = rotary_encoder_create_queue();
	ESP_ERROR_CHECK(rotary_encoder_set_queue(info, event_queue));
	ESP_LOGI(TAG, "rotary encoder inited");
	return ESP_OK;
};

esp_err_t RotaryEncoderModule::deinit(){

	ESP_ERROR_CHECK(rotary_encoder_uninit(info));
	ESP_LOGI(TAG, "deinited");
	return ESP_OK;
};

RotaryEncoderModule& RotaryEncoderModule::Instance(){

	static RotaryEncoderModule instance;
	return instance;
}
