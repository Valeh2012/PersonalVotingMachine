/**
 * @file VoteModel.cpp
 * @brief VoteModel implementation file
 * */

#include "model.h"

VoteModel::VoteModel(char* ssid, char* authToken, unsigned char* voteBase64, char*  choices){

	this->ssid = ssid;
	this->authToken = authToken;
	this->choices = choices;
	this->voteBase64 = voteBase64;
	this->voteID = NULL;
}
