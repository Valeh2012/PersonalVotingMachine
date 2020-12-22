/**
 * @file ScreenModule.cpp
 * @brief ScreenModule implementation file
 * */

#include "module.h"
#include "freertos/timers.h"
#include "esp_sleep.h"

#include "lvgl/lvgl.h"
#include "drv/disp_spi.h"
#include "drv/ili9341.h"

static void lv_task_timercb(void *timer){
    /* Periodically call this function.
     * The timing is not critical but should be between 1..10 ms */
    lv_task_handler();
}

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

	lv_init();

	disp_spi_init();
	ili9341_init();

	static lv_color_t buf1[DISP_BUF_SIZE];
	static lv_color_t buf2[DISP_BUF_SIZE];
	static lv_disp_buf_t disp_buf;
	lv_disp_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = ili9341_flush;
	disp_drv.buffer = &disp_buf;
	disp_drv.rotated = 1 ;
	lv_disp_drv_register(&disp_drv);

	esp_timer_create_args_t lv_task_timer_conf = {
		.callback = lv_task_timercb,
		.name     = "lv_task_timer"
	};
	esp_timer_handle_t lv_task_timer = NULL;
	esp_timer_create(&lv_task_timer_conf, &lv_task_timer);

	esp_timer_start_periodic(lv_task_timer, 5 * 1000U);

	return ESP_OK;
}

esp_err_t ScreenModule::deinit(){

	return ESP_OK;
}

ScreenModule& ScreenModule::Instance(){

	static ScreenModule instance;
	return instance;
}
