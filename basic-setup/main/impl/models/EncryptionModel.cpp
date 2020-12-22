/**
 * @file EncryptionModel.cpp
 * @brief EncryptionModel implementation file
 * */

#include "model.h"

EncryptionModel::EncryptionModel(char* ballot, char* ssid, char * authToken, const uint8_t keypem[], int keypem_length){

	this->ballot = ballot;
	this->ballotASN = NULL;
	this->ballotHash = NULL;
	this->ballotFileName = NULL;
	this->ballotLength = 0;
	this->rndBase64 = NULL;
	this->authToken = authToken;
	this->ssid = ssid;
	this->keypem = keypem;
	this->keypem_length = keypem_length;
	this->election_id = NULL;
}
