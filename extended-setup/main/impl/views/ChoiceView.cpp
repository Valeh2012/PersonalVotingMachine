/**
 * @file ChoiceView.cpp
 * @brief ChoiceView class implementation
 * */

#include "esp_log.h"
#include "string.h"

#include "lvgl/lvgl.h"
#include "module.h"
#include "view.h"
#include "model.h"

lv_obj_t * selected;
static bool nextstage = false;

static void btn_event_cb(lv_obj_t * btn, lv_event_t event){

	if(event == LV_EVENT_PRESSED) {
    	if(selected == NULL){
    		selected = btn;
    		lv_btn_set_state(btn, LV_BTN_STATE_TGL_PR);

    	}
    	else if(selected != btn){
    		lv_btn_set_state(selected, LV_BTN_STATE_REL);
    		selected = btn;
    		lv_btn_set_state(btn, LV_BTN_STATE_TGL_PR);
    	}else if(selected == btn){
    		lv_btn_set_state(btn, LV_BTN_STATE_PR);
    		selected= NULL;
    	}
    }
}

static void next_cb(lv_obj_t * btn, lv_event_t event){

    if(event == LV_EVENT_PRESSED) {
    	lv_obj_t * scr = lv_scr_act();
    	lv_obj_t * list1 = lv_obj_get_child(scr, NULL);
    	lv_list_ext_t * ext = (lv_list_ext_t *) lv_obj_get_ext_attr(list1);;
    	if(selected != NULL) {
    		nextstage = true;
    	}
    	else{
    		/*Create a window*/
			lv_obj_t * win = lv_win_create(lv_scr_act(), NULL);
			lv_win_set_title(win, "Window title");                        /*Set the title*/
			lv_obj_set_size(win, 240,120);
			lv_obj_align(win, NULL, LV_ALIGN_CENTER, 0, 0);

			/*Add control button to the header*/
			lv_obj_t * close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE);           /*Add close button and use built-in close action*/
			lv_obj_set_event_cb(close_btn, lv_win_close_event_cb);

			/*Add some dummy content*/
			lv_obj_t * txt = lv_label_create(win, NULL);
			lv_label_set_text(txt, "Select a candidate to continue\n");
    	}
    }
}

ChoiceView::ChoiceView(){};

void ChoiceView::render(void * data){

	if(data == NULL){
		lv_obj_t * scr = lv_obj_create(NULL, NULL);
		lv_obj_del(lv_scr_act());
		static lv_style_t style_scr_bg;
		lv_style_copy(&style_scr_bg, &lv_style_plain);
		style_scr_bg.body.main_color = lv_color_hex(0x487fb7);
		style_scr_bg.body.grad_color = lv_color_hex(0x487fb7);
		lv_obj_set_style(scr, &style_scr_bg);
		lv_obj_t * label1 = lv_label_create(scr, NULL);
		lv_label_set_text(label1, "Candidates");
		lv_label_set_align(label1, LV_LABEL_ALIGN_LEFT);
		lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
		lv_label_set_style(label1, LV_LABEL_STYLE_MAIN, &lv_style_plain_color);

		lv_style_copy(&big_text_style, &lv_style_plain_color);
		big_text_style.text.font = &lv_font_roboto_28;

		lv_obj_t * label2 = lv_label_create(scr, NULL);
		lv_label_set_text(label2, "");
		lv_label_set_long_mode(label2, LV_LABEL_LONG_BREAK);
		lv_obj_set_size(label2, 200, 200);
		lv_label_set_align(label2, LV_LABEL_ALIGN_CENTER);
		lv_obj_align(label2, NULL, LV_ALIGN_IN_LEFT_MID, 0, -20);
		lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, &big_text_style);
		lv_obj_set_hidden(label2, true);

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
		lv_obj_set_hidden(preload, false);

		lv_scr_load(scr);
	}else{

		ChoiceModel * model = (ChoiceModel *)data;

		lv_coord_t hres = lv_disp_get_hor_res(NULL);
		lv_coord_t vres = lv_disp_get_ver_res(NULL);

		lv_obj_t * scr = lv_scr_act();
		lv_obj_t * list1 = lv_list_create(scr, NULL);
		lv_obj_set_size(list1, hres, vres-80);
		lv_obj_align(list1, NULL, LV_ALIGN_IN_TOP_MID, 0, 40);
		static lv_style_t list_style;
		lv_style_copy(&list_style, &lv_style_btn_pr);
		list_style.body.main_color = LV_COLOR_RED;
		list_style.body.grad_color = lv_color_hex(0xff0088);
		lv_list_set_style(list1, LV_LIST_STYLE_BTN_TGL_PR, &list_style);
		lv_list_set_style(list1, LV_LIST_STYLE_BTN_TGL_REL, &list_style);
		/*Add buttons to the list*/

		lv_obj_t * list_btn;

		for(int i=0;i< model->choice_list->size(); i++){
			list_btn = lv_list_add_btn(list1, NULL, model->choice_list->at(i).candidate);
			lv_obj_set_height(list_btn, 20);
			lv_obj_set_event_cb(list_btn, btn_event_cb);
		}

		lv_list_set_sb_mode(list1,LV_SB_MODE_AUTO);

		lv_obj_t * btn1_label;
		lv_obj_t * btn1 = lv_btn_create(scr, NULL);
		lv_obj_set_event_cb(btn1, next_cb);
		lv_obj_set_size(btn1, 200, 40);
		lv_obj_align(btn1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

		btn1_label = lv_label_create(btn1, NULL);
		lv_label_set_style(btn1_label, LV_LABEL_STYLE_MAIN, &lv_style_plain_color);
		lv_label_set_text(btn1_label, "NEXT");

		while (!nextstage){
			// Wait for incoming events on the event queue.
			vTaskDelay(100);
		}

		int r = lv_list_get_btn_index(list1, selected);

//		  ballot = district-choice '\x1F' choicelist-name '\x1F' choice-name  //
		model->ballot = (char *) malloc(225);
		memset(model->ballot,0,225);
		snprintf(model->ballot, 224, "%s\x1F%s\x1F%s", model->choice_list->at(r).code, model->choice_list->at(r).party, model->choice_list->at(r).candidate);

	}
}

void ChoiceView::setLabel(char* data){

	for(int i=0;i<strlen(data);i++){
		if(data[i] == '_')
			data[i] = '\n';
	}

	lv_obj_t * scr = lv_scr_act();
	lv_obj_t * child = lv_obj_get_child_back(scr, NULL);
	child = lv_obj_get_child_back(scr, child);
	lv_label_set_text(child, data);
	lv_obj_set_hidden(child, false);
}

void ChoiceView::showLoader(bool en){

	lv_obj_t * scr = lv_scr_act();
	lv_obj_t * child = lv_obj_get_child(scr, NULL);
	lv_obj_set_hidden(child, !en);
}
