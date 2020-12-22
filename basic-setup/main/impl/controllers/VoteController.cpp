/**
 * @file VoteController.cpp
 * @brief VoteController implementation file
 * */

#include "cJSON.h"
#include "controller.h"

VoteController::VoteController(BaseModel *model, const char * sni){

	this->vw = new IndexView();
	this->model = static_cast<VoteModel *>(model);
	this->sni = sni;
}

void VoteController::index(){

	try{
		// send last hash to vote and get voteID
		ESP_LOGI(TAG, "final voting request sent");
		cJSON* voteJson = RPC::Instance().send_json_rpc(sni, generateVoteRequestJSON()); //! * send signed digital ballot to voting service

		if(voteJson == NULL){
			throw "Empty response";
		}
		if(cJSON_GetObjectItem(voteJson, "error")->valuestring != NULL){  //! * check error message
			throw cJSON_GetObjectItem(voteJson, "error")->valuestring;
		}

		cJSON* result = cJSON_GetObjectItem(voteJson, "result");  //! * if no error, parse result
		size_t len = strlen(cJSON_GetObjectItem(result, "VoteID")->valuestring);
		this->model->voteID = (char *) malloc(len+1);  //! * allocate memory and store voteID
		memset(this->model->voteID, 0, len+1);
		memcpy(this->model->voteID, cJSON_GetObjectItem(result, "VoteID")->valuestring,len);

		cJSON_Delete(voteJson);
		ESP_LOGI(TAG, "voteID: %s", this->model->voteID);
		this->vw->render((void *) "Vote casted");
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}

char* VoteController::generateVoteRequestJSON(){

	cJSON *root, *params, *param;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "id", 0.0);
	cJSON_AddItemToObject(root, "method", cJSON_CreateString("RPC.Vote"));

	params = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "params", params);

	cJSON_AddItemToArray(params, param=cJSON_CreateObject());
	cJSON_AddStringToObject(param, "AuthMethod", "ticket");
	cJSON_AddStringToObject(param, "AuthToken", this->model->authToken);
	cJSON_AddStringToObject(param, "Choices", this->model->choices);
	cJSON_AddStringToObject(param, "SessionID", this->model->ssid);
	cJSON_AddStringToObject(param, "OS", "FreeRTOS");
	cJSON_AddStringToObject(param, "Type", "bdoc");
	cJSON_AddStringToObject(param, "Vote", (char *) this->model->voteBase64);

	char* pretty = cJSON_Print(root);
	cJSON_Delete(root);

	return pretty;
}
