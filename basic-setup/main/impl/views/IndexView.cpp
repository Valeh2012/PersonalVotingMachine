#include "view.h"
#include "module.h"

/**
 * @file IndexView.cpp
 * @brief IndexView class implementation
 * */

IndexView::IndexView(){};

void IndexView::render(void * data){

	this->screen.print_screen((char *)data);
}
