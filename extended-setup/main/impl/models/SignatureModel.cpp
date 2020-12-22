/**
 * @file SignatureModel.cpp
 * @brief SignatureModel implementation file
 * */

#include "string.h"
#include "model.h"

SignatureModel::SignatureModel(char* ssid, char * authToken, char* phone, char* id, unsigned char* ballotHash, size_t ballotLength, char* ballotFileName){

	this->ssid  = ssid;
	this->authToken = authToken;
	this->sscode = NULL;

	memset(this->ID, 0, 12);
	memset(this->phone, 0, 13);
	memcpy(this->ID, id, 11);
	memcpy(this->phone, phone, 12);

	this->ballotHash = ballotHash;
	this->ballotLength = ballotLength;
	this->ballotFileName = ballotFileName;
	this->SignedInfoHash = NULL;
	this->signature = NULL;
	this->Signature = NULL;
	this->certificate = NULL;
	this->SI_XML = NULL;
	this->SP_XML = NULL;
	this->SV_XML = NULL;
}
