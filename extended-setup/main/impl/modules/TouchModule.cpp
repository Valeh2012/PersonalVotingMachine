/**
 * @file TouchModule.cpp
 * @brief TouchModule implementation file
 * */

#include "lvgl/lvgl.h"
#include "Arduino.h"
#include "TouchScreen.h"
#include "module.h"

#define TS_MINX 200
#define TS_MINY 150
#define TS_MAXX 940
#define TS_MAXY 950

TouchModule::TouchModule(gpio_num_t a, gpio_num_t b, gpio_num_t c, gpio_num_t d){

	this->YP = a;
	this->XP = b;
	this->XM = c;
	this->YM = d;
};

int map(int x, int in_min, int in_max, int out_min, int out_max) {

	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

TouchScreen* ts;

static bool touch_callback(lv_indev_drv_t * drv, lv_indev_data_t *data){

	static int last_x = 0, last_y = 0;
	analogReadResolution(10);
	TSPoint p = ts->getPoint();
	bool valid = true;
	int x,y;

	if(p.z > 0){
		p.x = 240 - map(p.x, TS_MINX, TS_MAXX, 0, 240);
		p.y = 320 - map(p.y, TS_MINY, TS_MAXY, 0, 320);
		x = p.x;
		y = p.y;
		last_x = x;
		last_y = y;
	}else{
		x = last_x;
		y = last_y;
		valid = false;
	}

	data->point.x = x;
	data->point.y = y;
	data->state = valid ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

	return false;
}

esp_err_t TouchModule::init(){

	ts = new TouchScreen(XP, YP, XM, YM, 0  );

	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);      /*Basic initialization*/
	indev_drv.type  =  LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = touch_callback;
	/*Register the driver in LittlevGL and save the created input device object*/
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);

	return ESP_OK;
};

esp_err_t TouchModule::deinit(){

	return ESP_OK;
};
