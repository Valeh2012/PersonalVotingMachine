#include "view.h"
#include "model.h"
#include "esp_log.h"
#include "module.h"
#include "string.h"

/**
 * @file ChoiceView.cpp
 * @brief ChoiceView class implementation
 * */

static int SCROLL_MAX = 6;
static int linestart = 0;
static int lineend = SCROLL_MAX;

static int min(int a, int b){
	if(a < b) return a;
	return b;
}

ChoiceView::ChoiceView(){};

void ChoiceView::render(void * data){

	ChoiceModel * model = (ChoiceModel *) data;

	int r = 0;
	this->list(*model->choice_list, r);

	QueueHandle_t event_queue = rotary_encoder_create_queue();
	ESP_ERROR_CHECK(rotary_encoder_set_queue(RotaryEncoderModule::Instance().getInfo(), event_queue));

	int choiceSelected = 0;
	while (!choiceSelected)
	{
		// Wait for incoming events on the event queue.
		rotary_encoder_event_t *event = new rotary_encoder_event_t();
		if (xQueueReceive(event_queue, event, 1000 / portTICK_PERIOD_MS) == pdTRUE)
		{
			if(event->state.clicked ){
				ESP_LOGI("RotEnc", "Event %s\n", event->state.clicked ? "clicked":"not clicked");
				choiceSelected = 1;
				break;
			}
			else{
				ESP_LOGI("RotEnc", "Event: position %d, direction %s", event->state.position,
						event->state.direction ? (event->state.direction == ROTARY_ENCODER_DIRECTION_CLOCKWISE ? "CW" : "CCW") : "NOT_SET");
				r += event->state.direction == ROTARY_ENCODER_DIRECTION_CLOCKWISE ? -1 : 1 ;

				if(r <= 0) r = 0;
				if(r >= model->choice_list->size()) r = model->choice_list->size()-1;

				if(r >= lineend ){ lineend = r+1; linestart = lineend - SCROLL_MAX; }
				if(r <= linestart ){ linestart = r;  lineend = linestart + SCROLL_MAX; }
				this->list(*model->choice_list, r);
			}
		}
	}

	//  ballot = district-choice '\x1F' choicelist-name '\x1F' choice-name  //
	model->ballot = (char *) malloc(225);
	memset(model->ballot,0,225);
	snprintf(model->ballot, 224, "%s\x1F%s\x1F%s", (*model->choice_list)[r].code, (*model->choice_list)[r].party, (*model->choice_list)[r].candidate);
}

void ChoiceView::list(std::vector<choice_t> &v,int r){

	this->screen.clear();
	for(int i=linestart;i< min(lineend, (int) v.size()); i++){
		this->screen.writeLine(v[i].candidate, i==r ? 10 : 0);
	}
	this->screen.FillCircle(120, (r - linestart)*10+5,5);
	this->screen.refresh();
}



