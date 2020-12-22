/**
 * @file ZipModel.cpp
 * @brief ZipModel implementation file
 * */

#include "string.h"
#include "model.h"

ZipModel::ZipModel(unsigned char* ballot, char* ballotFileName, size_t ballotLength, char* Signature){

	this->ballot = ballot;
	this->ballotFileName = ballotFileName;
	this->ballotLength = ballotLength;
	this->Signature = Signature;

	this->authToken = NULL;
	this->ssid = NULL;
	this->voteBase64 = NULL;
}
