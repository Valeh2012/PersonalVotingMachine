/**
 * @file IndexView.cpp
 * @brief IndexView class implementation
 * */

#include "lvgl/lvgl.h"
#include "view.h"
#include "qrcode.h"

static bool clicked = false;

static void finish_cb(lv_obj_t * btn, lv_event_t event){

	if(event == LV_EVENT_CLICKED){
		clicked = true;
	}
}

QRView::QRView(){};

void QRView::render(void * data){

	QRCode * qrcode = (QRCode *) data;
	lv_obj_t * scr = lv_obj_create(NULL, NULL);

	lv_style_copy(&style_scr_bg, &lv_style_plain);
	style_scr_bg.body.main_color = lv_color_hex(0x00ff00);
	style_scr_bg.body.grad_color = lv_color_hex(0x00ff00);
	lv_obj_set_style(scr, &style_scr_bg);

	lv_obj_t * label1 = lv_label_create(scr, NULL);
	lv_label_set_text(label1, (char *) "QR code");
	lv_label_set_align(label1, LV_LABEL_ALIGN_CENTER);
	lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);

	uint8_t CH = 2*qrcode->size;
	static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_INDEXED_1BIT(240, 240)];
	lv_obj_t * canvas = lv_canvas_create(scr, NULL);
	lv_canvas_set_buffer(canvas, cbuf, 240,240, LV_IMG_CF_INDEXED_1BIT);
	lv_canvas_set_palette(canvas, 0, LV_COLOR_WHITE);
	lv_canvas_set_palette(canvas, 1, LV_COLOR_BLACK);
	lv_obj_align(canvas, NULL, LV_ALIGN_CENTER, 0, 0);
	/*Create colors with the indices of the palette*/
	lv_color_t c0;
	lv_color_t c1;

	c0.full = 0;
	c1.full = 1;

	lv_canvas_fill_bg(canvas, c0);

	static lv_style_t small_text_style;
	lv_style_copy(&small_text_style, &lv_style_plain);
	small_text_style.text.font = &lv_font_roboto_12;
	small_text_style.text.color = c1;

	uint8_t x2=10,y2=10;
	for (uint8_t y = 0; y < qrcode->size; y++) {
		for (uint8_t x = 0; x < qrcode->size; x++) {
			if (qrcode_getModule(qrcode, x, y)) {
				lv_canvas_set_px(canvas, 2*x+5, 2*y+5, c1);
				lv_canvas_set_px(canvas, 2*x+1+5, 2*y+5, c1);
				lv_canvas_set_px(canvas, 2*x+5, 2*y+1+5, c1);
				lv_canvas_set_px(canvas, 2*x+1+5, 2*y+1+5, c1);
			}
			x2+=2;
			if(x2 == 2*qrcode->size){
				y2+=2;
				x2=10;
			}
		}
	}

	lv_obj_t * label2;
	lv_obj_t * btn1 = lv_btn_create(scr, NULL);
	lv_obj_set_event_cb(btn1, finish_cb);
	lv_obj_set_size(btn1, 100, 40);
	lv_obj_align(btn1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

	label2 = lv_label_create(btn1, NULL);
	lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, &lv_style_plain_color);
	lv_label_set_text(label2, "Finish");

	lv_obj_del(lv_scr_act());
	lv_scr_load(scr);

	while(!clicked){
		vTaskDelay(100);
	}
}

void QRView::setLabel(char* data){

}

void QRView::showLoader(bool en){

}
