/**
 * @file AuthorizationController.cpp
 * @brief AuthorizationController implementation file
 * */

#include "cJSON.h"
#include "controller.h"
#include "module.h"

AuthorizationController::AuthorizationController(BaseModel *model, const char* sni){

	this->model = static_cast<UserModel *>(model);
	this->sni = sni;
	this->vw = new IndexView();
}

char* AuthorizationController::generateAuthenticateRequestJSON(){

	cJSON *root, *params, *param;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "id", 0.0);
	cJSON_AddItemToObject(root, "method", cJSON_CreateString("RPC.Authenticate"));

	params = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "params", params);

	cJSON_AddItemToArray(params, param = cJSON_CreateObject());
	cJSON_AddStringToObject(param, "OS", "FreeRTOS");
	cJSON_AddStringToObject(param, "PhoneNo", this->model->phone);
	cJSON_AddStringToObject(param, "IDCode", this->model->ID);

	char* pretty = cJSON_Print(root);
	cJSON_Delete(root);

	return pretty;
}

char* AuthorizationController::generateAuthenticateStatusRequestJSON(){

	cJSON *root, *params, *param;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "id", 0.0);
	cJSON_AddItemToObject(root, "method", cJSON_CreateString("RPC.AuthenticateStatus"));

	params = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "params", params);

	cJSON_AddItemToArray(params, param = cJSON_CreateObject());
	cJSON_AddStringToObject(param, "OS", "FreeRTOS");
	cJSON_AddStringToObject(param, "SessionID", this->model->ssid);
	cJSON_AddStringToObject(param, "SessionCode", this->model->sscode);

	char* pretty = cJSON_Print(root);
	cJSON_Delete(root);

	return pretty;
}

void AuthorizationController::index(){

	this->vw->render((char *) "Authorization");
}

void AuthorizationController::auth(){

	try{

		// Start mobilID Authentication
		ESP_LOGI(TAG, "authorization request sent");
		cJSON *authJson = RPC::Instance().send_json_rpc(sni, generateAuthenticateRequestJSON());

		if(authJson == NULL){
			throw "Empty response";
		}
		if( cJSON_GetObjectItem(authJson, "error")->valuestring != NULL){
			throw cJSON_GetObjectItem(authJson, "error")->valuestring;
		}

		cJSON* params = cJSON_GetObjectItem(authJson, "result");

		int len = strlen(cJSON_GetObjectItem(params, "SessionID")->valuestring);
		this->model->ssid = (char *) malloc(len +1);
		memset(this->model->ssid, 0, len + 1);
		memcpy(this->model->ssid, cJSON_GetObjectItem(params, "SessionID")->valuestring, len);

		len = strlen(cJSON_GetObjectItem(params, "SessionCode")->valuestring);
		this->model->sscode = (char *) malloc(len +1);
		memset(this->model->sscode, 0, len + 1);
		memcpy(this->model->sscode, cJSON_GetObjectItem(params, "SessionCode")->valuestring, len);

		char pin[25];
		bzero(pin, 25);
		strncat(pin, "Verification code: ", 20);
		strncat(pin, cJSON_GetObjectItem(params, "ChallengeID")->valuestring, 5);

		cJSON_Delete(authJson);

		this->vw->render(pin);
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}

void AuthorizationController::authStatus(){

	try{

		char* authStatusReq = generateAuthenticateStatusRequestJSON();
		cJSON * result, * authJson;
		do{
			ESP_LOGI(TAG, "waiting to confirm verification code");
			for(int i=10; i>0; i--){
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}

			// Check authentication status
			ESP_LOGI(TAG, "authorization status request sent");
			authJson = RPC::Instance().send_json_rpc(sni, authStatusReq );
			if(authJson == NULL){
				throw "Empty response";
			}

			result = cJSON_GetObjectItem(authJson, "result");
			if( cJSON_GetObjectItem(authJson, "error")->valuestring != NULL){
				throw cJSON_GetObjectItem(authJson, "error")->valuestring;
			}
			if(strcmp(cJSON_GetObjectItem(result, "Status")->valuestring, "POLL") == 0){
				continue;
			}
			if(strcmp(cJSON_GetObjectItem(result, "Status")->valuestring, "OK") == 0){
				int len = strlen(cJSON_GetObjectItem(result, "AuthToken")->valuestring);
				this->model->authToken = (char *)malloc(len + 1);
				memset(this->model->authToken, 0, len + 1);
				memcpy(this->model->authToken, cJSON_GetObjectItem(result, "AuthToken")->valuestring, len);
				this->vw->render((char *) "PIN CONFIRMED!");
				break;
			}
			cJSON_free(authJson);
		}
		while(1);

		cJSON_Delete(authJson);
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}
