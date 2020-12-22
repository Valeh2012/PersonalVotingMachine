/**
 * @file QRModel.cpp
 * @brief QRModel implementation file
 * */

#include "model.h"

QRModel::QRModel(char* ssid, char* voteID, unsigned char* rndBase64){

	this->ssid = ssid;
	this->voteID = voteID;
	this->rndBase64 = rndBase64;
}
