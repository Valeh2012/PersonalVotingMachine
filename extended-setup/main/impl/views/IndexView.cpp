/**
 * @file IndexView.cpp
 * @brief IndexView class implementation
 * */

#include "lvgl/lvgl.h"
#include "view.h"

IndexView::IndexView(){};

void IndexView::render(void * data){

	lv_obj_t * scr = lv_obj_create(NULL, NULL);
	lv_obj_del(lv_scr_act());

	lv_style_copy(&style_scr_bg, &lv_style_plain);
	style_scr_bg.body.main_color = lv_color_hex(0x487fb7);
	style_scr_bg.body.grad_color = lv_color_hex(0x487fb7);
	lv_obj_set_style(scr, &style_scr_bg);

	lv_obj_t * label1 = lv_label_create(scr, NULL);
	lv_label_set_text(label1, (char *) data);
	lv_label_set_align(label1, LV_LABEL_ALIGN_CENTER);
	lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
	lv_label_set_style(label1, LV_LABEL_STYLE_MAIN, &lv_style_plain_color);

	lv_style_copy(&big_text_style, &lv_style_plain_color);
	big_text_style.text.font = &lv_font_roboto_22;

	lv_obj_t * label2 = lv_label_create(scr, NULL);
	lv_label_set_text(label2, "");
	lv_label_set_long_mode(label2, LV_LABEL_LONG_BREAK);
	lv_obj_set_size(label2, 200, 200);
	lv_label_set_align(label2, LV_LABEL_ALIGN_CENTER);
	lv_obj_align(label2, NULL, LV_ALIGN_IN_LEFT_MID, 0, -20);
	lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, &big_text_style);

    /*Create a style for the Preloader*/
	lv_style_copy(&style, &lv_style_plain);
	style.line.width = 4;                         /*10 px thick arc*/
	style.line.color = lv_color_hex3(0xfff);       /*Blueish arc color*/

	style.body.border.color = lv_color_hex(0x487fb7); /*Gray background color*/
	style.body.border.width = 4;
	style.body.padding.left = 0;

	/*Create a Preloader object*/
	lv_obj_t * preload = lv_preload_create(scr, NULL);
	lv_preload_set_type(preload, LV_PRELOAD_TYPE_FILLSPIN_ARC);
	lv_obj_set_size(preload, 60, 60);
	lv_preload_set_spin_time(preload, 1000);
	lv_preload_set_arc_length(preload, 120);
	lv_obj_align(preload, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -5);
	lv_preload_set_style(preload, LV_PRELOAD_STYLE_MAIN, &style);
	lv_obj_set_hidden(preload, true);

	lv_scr_load(scr);
}

void IndexView::setLabel(char* data){

	for(int i=0;i<strlen(data);i++){
		if(data[i] == '_'){
			data[i] = '\n';
		}
	}
	lv_obj_t * scr = lv_scr_act();
	lv_obj_t * child = lv_obj_get_child(scr, NULL);
	child = lv_obj_get_child(scr, child);
	lv_label_set_text(child, data);
}

void IndexView::showLoader(bool en){

	lv_obj_t * scr = lv_scr_act();
	lv_obj_t * child = lv_obj_get_child(scr, NULL);
	lv_obj_set_hidden(child, !en);
}
