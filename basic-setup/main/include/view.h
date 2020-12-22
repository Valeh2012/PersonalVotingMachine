#ifndef MAIN_INCLUDE_VIEW_H_
#define MAIN_INCLUDE_VIEW_H_

/**
 * @file view.h
 * @brief View class
 * */

#include "module.h"
#include "model.h"

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

	/**
	 * @brief show all candidates
	 *
	 * Given list of candidates, this method print name of candidates on screen with small circle to highlight
	 * "cursor" location, in other words hovered element.
	 *
	 * @param v list of choices
	 * @param r position to draw circle
	 * */
	void list(std::vector<choice_t>& v, int r);
};


#endif /* MAIN_INCLUDE_VIEW_H_ */
