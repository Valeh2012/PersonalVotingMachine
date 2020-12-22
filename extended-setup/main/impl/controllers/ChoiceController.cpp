/**
 * @file ChoiceController.cpp
 * @brief ChoiceController implementation file
 * */

#include "cJSON.h"
#include "controller.h"

ChoiceController::ChoiceController(BaseModel *model, const char* sni){

	this->vw = new ChoiceView();
	this->model = static_cast<ChoiceModel*>(model);
	this->sni = sni;
}

void ChoiceController::index(){

	try{
		this->vw->render(NULL);
		char* choicesRequestJson = generateChoicesRequestJSON(this->model);
		// Get choices from server
		cJSON *choicesJson = RPC::Instance().send_json_rpc(sni, choicesRequestJson);
		cJSON *result;
		if(choicesJson == NULL){
			throw "Empty response";
		}
		if( cJSON_GetObjectItem(choicesJson, "error")->valuestring != NULL){
			throw cJSON_GetObjectItem(choicesJson, "error")->valuestring;
		}

		result = cJSON_GetObjectItem(choicesJson, "result");

		this->model->choices = (char *) malloc(21);
		memset(this->model->choices, 0, 21);
		strncpy(this->model->choices, cJSON_GetObjectItem(result, "Choices")->valuestring, 21);

		unsigned char* list = (unsigned char *) cJSON_GetObjectItem(result, "List")->valuestring;

		size_t bufflen = strlen((char*) list);
		bufflen = (bufflen/4) *3 +1; // length before base64 encoding

		unsigned char *decoded_list = (unsigned char *) malloc(bufflen);
		if(decoded_list == NULL) throw "insufficient memory";
		memset(decoded_list, 0, bufflen);

		size_t len;

		int error =  mbedtls_base64_decode(decoded_list, bufflen, &len, list, strlen( (char*) list));
		if(error) throw "Error decoding choice list";

		cJSON *parsedList = cJSON_Parse((char *) decoded_list);
		parse_object(parsedList, (char *)"" );
		ESP_LOGI(TAG, "total candidates: %d", this->model->choice_list->size());

		this->vw->showLoader(false);
		this->vw->render((void *) this->model);
		cJSON_Delete(parsedList);
		free(decoded_list);
		cJSON_Delete(choicesJson);
	}
	catch(const char* msg){
		this->vw->setLabel((char *) msg);
		this->vw->showLoader(false);
		throw msg;
	}
}

char* ChoiceController::generateChoicesRequestJSON(ChoiceModel *cm){

	cJSON *root, *params, *param;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "id", 0.0);
	cJSON_AddItemToObject(root, "method", cJSON_CreateString("RPC.VoterChoices"));

	params = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "params", params);

	cJSON_AddItemToArray(params, param=cJSON_CreateObject());
	cJSON_AddStringToObject(param, "AuthMethod", "ticket");
	cJSON_AddStringToObject(param, "AuthToken", cm->authToken);
	cJSON_AddStringToObject(param, "OS", "Operating System,2,0");
	cJSON_AddStringToObject(param, "SessionID", cm->ssid);

	char* pretty = cJSON_Print(root);
	cJSON_Delete(root);

	return pretty;
}


void ChoiceController::parse_object(cJSON *item, char* par){

	cJSON *subitem=item->child;
	if ( !subitem ) {  // leaf nodes are candidates
		while(item){

			choice_t *c = new choice_t();  // create new choice element

			c->candidate = (char *) malloc(strlen(item->valuestring)+1);  // allocate memory for candidate field
			memset(c->candidate, 0, strlen(item->valuestring) +1);        // set content 0
			memcpy(c->candidate, item->valuestring, strlen(item->valuestring)); // copy from json to struct field

			c->code = (char *) malloc(strlen(item->string)+1);  // allocate memory for code field
			memset(c->code, 0, strlen(item->string) +1);	   // set content 0
			memcpy(c->code, item->string, strlen(item->string));  // copy from json to struct field

			c->party = (char *) malloc(strlen(par)+1);			// allocate memory for party field
			memset(c->party, 0, strlen(par) +1);                 // set content 0
			memcpy(c->party, par, strlen(par));					// copy from json to struct field

			this->model->choice_list->push_back(*c);   	// push choice element to vector (list of choices)

			item = item->next;  // go to next candidate
		}
	}
	while (subitem){
		// handle subitem
		if (subitem->child) {
			parse_object(subitem->child, subitem->string);
		}
		subitem=subitem->next;
	}
}
