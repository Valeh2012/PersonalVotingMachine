/**
 * @file ChoiceModel.cpp
 * @brief ChoiceModel implementation file
 * */

#include "model.h"

ChoiceModel::ChoiceModel(char* ssid, char * authToken){

	this->ssid = ssid;
	this->authToken = authToken;
	this->choices = NULL;
	this->choice_list = new std::vector<choice_t>();
	this->ballot  = NULL;
}
