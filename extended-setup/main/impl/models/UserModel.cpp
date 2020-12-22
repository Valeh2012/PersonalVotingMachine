/**
 * @file UserModel.cpp
 * @brief UserModel implementation file
 * */

#include "string.h"
#include "model.h"

UserModel::UserModel(char* id, char* phone){

	memset(this->ID, 0, 12);
	memset(this->phone, 0, 13);
	memcpy(this->ID, id, 11);
	memcpy(this->phone, phone, strlen(phone));

	this->ssid = NULL;
	this->authToken = NULL;
	this->sscode = NULL;
}
