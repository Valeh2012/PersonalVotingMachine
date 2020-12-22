/**
 * @file IndexController.cpp
 * @brief IndexController implementation file
 * */

#include "controller.h"

IndexController::IndexController(){

	this->vw = new IndexView();
}

void IndexController::index(){

	this->vw->render((void *) "Index ctrl");
}
