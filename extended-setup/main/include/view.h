#ifndef MAIN_INCLUDE_VIEW_H_
#define MAIN_INCLUDE_VIEW_H_

/**
 * @file view.h
 * @brief View class
 * */

#include <vector>
#include "lvgl/lvgl.h"
#include "module.h"
#include "model.h"

static lv_style_t style_scr_bg;
static lv_style_t big_text_style;
static lv_style_t headline_style;
static lv_style_t style;

/**
 * \class View
 * \brief V in MVC pattern. Each controller is associated with a view.
 * There is only one screen device attached to board. This view helps to display data and accept user input
 * */
class View{
public:
	ScreenModule screen = ScreenModule::Instance();  /**< singleton screen instance */
	virtual void render(void* data) =  0;  /**< abstract method to render data*/
	virtual ~View(){};  // destructor
	virtual void setLabel(char* data) = 0;
	virtual void showLoader(bool en) = 0;
};

/**
 *  \class IndexView
 *  @brief default inherited class
 * */
class IndexView : public View{
public:
	IndexView();

	/**
	 *  @brief display data on screen
	 *
	 *  @param data (void *) casted string buffer to display
	 * */
	void render(void * data);
	void setLabel(char* data);
	void showLoader(bool en);
};

/**
 *  \class ChoiceView
 *  @brief more functional view class that ChoiceController uses.
 * */
class ChoiceView : public View{
public:
	ChoiceView();

	/**
	 * @brief render ChoiceController data
	 *
	 * Here, users allowed to interact with device using RotaryEncoder. Every time encoder state is changed,
	 * screen is updated. A small circle in front of candidate names represent selected option index. This method
	 * will listen rotary encoder until click event is fired, and then create ballot based on selected candidate data.
	 *
	 * @param data (void *) casted ChoiceModel * model.
	 * */
	void render(void * data);
	void setLabel(char* data);
	void showLoader(bool en);
};

class QRView : public View{
public:
	QRView();

	/**
	 *  @brief display data on screen
	 *
	 *  @param data (void *) casted string buffer to display
	 * */
	void render(void * data);
	void setLabel(char* data);
	void showLoader(bool en);
};
#endif /* MAIN_INCLUDE_VIEW_H_ */
