/**
 * @file UserModel.cpp
 * @brief UserModel implementation file
 * */

#include "string.h"
#include "model.h"

UserModel::UserModel(char* id, char* phone){

	memset(this->ID, 0, 12);
	memset(this->phone, 0, 12);
	strncpy(this->ID, id, 11);
	strncpy(this->phone, phone, 12);

	ssid = NULL;
	authToken = NULL;
	sscode = NULL;
}
