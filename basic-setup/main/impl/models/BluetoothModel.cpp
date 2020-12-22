/**
 * @file BluetoothModel.cpp
 * @brief BluetoothModel implementation file
 * */

#include "model.h"

BluetoothModel::BluetoothModel(char *ssid, char* voteID, unsigned char * rndBase64){

	this->ssid = ssid;
	this->voteID = voteID;
	this->rndBase64 = rndBase64;
}
