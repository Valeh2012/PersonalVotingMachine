/**
 * @file IndexController.cpp
 * @brief IndexController implementation file
 * */

#include "controller.h"

IndexController::IndexController(){

	this->vw = new IndexView();
}

void IndexController::index(){

	this->vw->render((void *) "Welcome\n");
	this->vw->setLabel("starting");
}
void IndexController::end(){

	this->vw->render((void *) "End\n");
	this->vw->setLabel("Click boot to start again");
	this->vw->showLoader(false);
}
