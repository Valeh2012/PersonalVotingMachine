/**
 * @file SignatureController.cpp
 * @brief SignatureController implementation file
 * */

#include "cJSON.h"
#include "controller.h"
#include "converter.h"

SignatureController::SignatureController(BaseModel *model, const char * sni){

	this->vw = new IndexView();
	this->model = static_cast<SignatureModel *>(model);
	this->sni = sni;
}

void SignatureController::index(){

	try{
		this->vw->render((void *) "Getting certificate");

		// Get certificate
		ESP_LOGI(TAG, "get certificate request sent");
		cJSON *certificateJson = RPC::Instance().send_json_rpc(sni, generateGetCertificateRequestJSON());  //! * Get certificate from server
		if(certificateJson == NULL){
			throw "Empty response";
		}
		if( cJSON_GetObjectItem(certificateJson, "error")->valuestring != NULL){  //! * check for error message
			throw cJSON_GetObjectItem(certificateJson, "error")->valuestring;
		}

		cJSON * result = cJSON_GetObjectItem(certificateJson, "result");  //! * if no error, parse JSON result
		size_t len = strlen(cJSON_GetObjectItem(result, "Certificate")->valuestring);
		char * signCertificate = (char *)malloc(len + 1);  //! * allocate enough memory to store certificate
		if(signCertificate == NULL){
			ESP_LOGE("ivxv", "unable to allocate memory to store certificate");
			throw "Insufficient memory";
		}
		memset(signCertificate, 0, len + 1);
		memcpy(signCertificate, cJSON_GetObjectItem(result, "Certificate")->valuestring, len);

		len = (len/64) *65 + len%64;
		this->model->certificate = (char *)malloc(len+1);
		memset(this->model->certificate,0, len+1);  //! * base64 decode  and add line breaks after 64th character
		int i, j=0;
		for(i=0;i<strlen(signCertificate);i++){
			this->model->certificate[j++] = signCertificate[i];
			if(i && (i+1)%64==0){
				this->model->certificate[j++] = '\n';
			}
		}
		cJSON_Delete(certificateJson);

		// ASN1 Parse certificate
		// decode certificate, parse it, hash it, base64 encode it.
		unsigned char *certificateDER = (unsigned char *) malloc(1600);
		if(certificateDER == NULL){
			ESP_LOGE("ivxv", "unable to allocate memory to store certificate DER");
			throw "Insufficient memory";
		}
		memset(certificateDER, 0, 1600);

		int error = mbedtls_base64_decode(certificateDER, 1600, &len, (unsigned char *)signCertificate, strlen(signCertificate));
		if(error) throw "Error";
		int der_length = len;

		unsigned char* p = certificateDER;
		unsigned char* end = p + len;
		int version;
		mbedtls_mpi serial = {0,0,0};
		char * serialBuf = (char *)malloc(128);
		memset(serialBuf, 0 , 128);
		error = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
		if(error) throw "Error";
		error = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
		if(error) throw "Error";
		error = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC);
		if(error) throw "Error";
		error = mbedtls_asn1_get_int(&p, end, &version);
		if(error) throw "Error";
		error = mbedtls_asn1_get_mpi(&p, end, &serial);
		if(error) throw "Error";
		error = mbedtls_mpi_write_string(&serial, 10, serialBuf, 128, &len);   //! * parse certificate serial number
		if(error) throw "Error";

		mbedtls_asn1_buf  oid, oid_params;
		memset( &oid, 0, sizeof( mbedtls_asn1_buf ) );
		memset( &oid_params, 0, sizeof( mbedtls_asn1_buf ) );
		error = mbedtls_asn1_get_alg_null(&p, end, &oid);
		if(error) throw "Error";
		error = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
		if(error) throw "Error";

		char *DN = (char *)malloc(256);
		memset(DN,0, 256);

		char* CN = (char *) malloc(32);
		memset(CN,0, 32);

		char* C = (char *) malloc(32);
		memset(C,0, 32);

		char* O = (char *) malloc(32);
		memset(O,0, 32);

		char* org = (char *) malloc(32);
		memset(org,0, 32);

		for(i=0;i<4;i++){
			memset( &oid, 0, sizeof( mbedtls_asn1_buf ) );
			memset( &oid_params, 0, sizeof( mbedtls_asn1_buf ) );
			error = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET);
			if(error) {
				throw "Error";
			}
			error = mbedtls_asn1_get_alg(&p, end, &oid, &oid_params);
			if(error) {
				throw "Error";
			}
			if( *(oid.p + 2) == 0x03) strncat(CN, (char *) oid_params.p, oid_params.len);
			if( *(oid.p + 2) == 0x06) strncat(C, (char *) oid_params.p, oid_params.len);
			if( *(oid.p + 2) == 0x0A) strncat(O, (char *) oid_params.p, oid_params.len);
			if( *(oid.p + 2) == 0x61) strncat(org, (char *) oid_params.p, oid_params.len);


		}
		if(error) throw "Error";
		snprintf(DN, 256, "CN=%s,organizationIdentifier=%s,O=%s,C=%s", CN, org, O, C);  // copy certificate DN field

		unsigned char *certificateHashBase64 = converter::base64hash(certificateDER, der_length);

		free(certificateDER);
		free(signCertificate);

		// Create signedProperties
		char *signingTime = (char *) malloc(64);
		memset(signingTime, 0, 64);
		time_t now;
		time(&now);
		strftime(signingTime, 21,"%Y-%m-%dT%XZ", localtime(&now));   //! * get signing time

		ESP_LOGI(TAG, "time: %s", signingTime);

		this->model->SP_XML = (char *) malloc(2048);
		memset(this->model->SP_XML, 0, 2048);
		snprintf(this->model->SP_XML, 2048, SignedProperties, signingTime, (char *)certificateHashBase64, DN, serialBuf);  //! * create SignedProperties XML element

		converter::xmlc14n11(this->model->SP_XML, 2048);  //! * canonicalize SignedProperties

		unsigned char *spHashBase64 = converter::base64hash((unsigned char *)this->model->SP_XML, strlen(this->model->SP_XML));  //! * hash (sha256) + base64 encode canonicalize

		// Create signedInfo
		this->model->SI_XML = (char *) malloc(2048);
		memset(this->model->SI_XML, 0, 2048);
		snprintf(this->model->SI_XML, 2048, SignedInfo, this->model->ballotFileName ,(char *) this->model->ballotHash, spHashBase64); //! create DignedInfo XML element

		converter::xmlc14n11(this->model->SI_XML, 2048);

		this->model->SignedInfoHash = converter::base64hash((unsigned char *)this->model->SI_XML, strlen(this->model->SI_XML));//! * hash(sha268) andbase64 encode canonicalize
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}

char* SignatureController::generateGetCertificateRequestJSON(){

	cJSON *root, *params, *param;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "id", 0.0);
	cJSON_AddItemToObject(root, "method", cJSON_CreateString("RPC.GetCertificate"));

	params = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "params", params);

	cJSON_AddItemToArray(params, param=cJSON_CreateObject());
	cJSON_AddStringToObject(param, "AuthMethod", "ticket");
	cJSON_AddStringToObject(param, "AuthToken", this->model->authToken);
	cJSON_AddStringToObject(param, "OS", "FreeRTOS");
	cJSON_AddStringToObject(param, "PhoneNo", this->model->phone);
	cJSON_AddStringToObject(param, "IDCode", this->model->ID);
	cJSON_AddStringToObject(param, "SessionID", this->model->ssid);

	char* pretty = cJSON_Print(root);
	cJSON_Delete(root);

	return pretty;
}

char* SignatureController::generateSignRequestJSON(){

	cJSON *root, *params, *param;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "id", 0.0);
	cJSON_AddItemToObject(root, "method", cJSON_CreateString("RPC.Sign"));

	params = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "params", params);

	cJSON_AddItemToArray(params, param=cJSON_CreateObject());
	cJSON_AddStringToObject(param, "AuthMethod", "ticket");
	cJSON_AddStringToObject(param, "AuthToken", this->model->authToken);
	cJSON_AddStringToObject(param, "Hash", (char *) this->model->SignedInfoHash);
	cJSON_AddStringToObject(param, "OS", "FreeRTOS");
	cJSON_AddStringToObject(param, "PhoneNo", this->model->phone);
	cJSON_AddStringToObject(param, "IDCode", this->model->ID);
	cJSON_AddStringToObject(param, "SessionID", this->model->ssid);

	char* pretty = cJSON_Print(root);
	cJSON_Delete(root);

	return pretty;
}

void SignatureController::sign(){

	try{
		ESP_LOGI(TAG, "signing request sent");
		cJSON * signJson = RPC::Instance().send_json_rpc(sni, generateSignRequestJSON());

		if(signJson == NULL){
			throw "Empty response";
		}
		if(cJSON_GetObjectItem(signJson, "error")->valuestring != NULL){
			throw cJSON_GetObjectItem(signJson, "error")->valuestring;
		}
		cJSON * params = cJSON_GetObjectItem(signJson, "result");

		size_t len = strlen(cJSON_GetObjectItem(params, "SessionCode")->valuestring);
		this->model->sscode = (char *) malloc(len +1);
		memset(this->model->sscode, 0, len + 1);
		memcpy(this->model->sscode, cJSON_GetObjectItem(params, "SessionCode")->valuestring, len);

		char pin[25];
		bzero(pin, 25);
		strncat(pin, "Verification code: ", 20);
		strncat(pin, cJSON_GetObjectItem(params, "ChallengeID")->valuestring, 5);
		this->vw->render((void *) pin);

		cJSON_Delete(signJson);
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}

char* SignatureController::generateSignStatusRequestJSON(){

	cJSON *root, *params, *param;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "id", 0.0);
	cJSON_AddItemToObject(root, "method", cJSON_CreateString("RPC.SignStatus"));

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

void SignatureController::status(){

	try{
		// Poll RPC.SignStatus
		cJSON *signJson, *result;
		char* signStatusReq = generateSignStatusRequestJSON();
		do{
			ESP_LOGI(TAG, "waiting to confirm pin");
			for(int i=10; i>0; i--){
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}

			// check status
			ESP_LOGI(TAG, "signing status request sent");
			signJson = RPC::Instance().send_json_rpc(sni, signStatusReq );
			if(signJson == NULL){
				throw "Empty response";
			}

			result = cJSON_GetObjectItem(signJson, "result");
			if( cJSON_GetObjectItem(signJson, "error")->valuestring != NULL){
				throw cJSON_GetObjectItem(signJson, "error")->valuestring;
			}
			if(strcmp(cJSON_GetObjectItem(result, "Status")->valuestring, "POLL") == 0){
				continue;
			}
			if(strcmp(cJSON_GetObjectItem(result, "Status")->valuestring, "OK") == 0){
				size_t len = strlen(cJSON_GetObjectItem(result, "Signature")->valuestring);
				this->model->signature = (char *) malloc(len + 1);
				memset(this->model->signature, 0, len + 1);
				memcpy(this->model->signature, cJSON_GetObjectItem(result, "Signature")->valuestring, len);
				this->vw->render((void *) "PIN CONFIRMED!");
				break;
			}

			cJSON_free(signJson);
		}
		while(1);
		this->vw->render((void *)"Successfully signed");
		ESP_LOGI(TAG, "succesfully signed");

		cJSON_Delete(signJson);
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}

void SignatureController::combine(){

	try{
		// Create SignatureValue
		this->model->SV_XML = (char *) malloc(512);
		memset(this->model->SV_XML, 0, 512);
		snprintf(this->model->SV_XML, 511, SignatureValue, this->model->signature);

		// merge above 3 and create signature0.xml
		this->model->Signature = (char *) malloc(10240);
		if(this->model->Signature == NULL){
			throw "Insufficient memory";
		}

		memset(this->model->Signature, 0, 10240);
		snprintf(this->model->Signature, 10240, Signature, this->model->SI_XML, this->model->SV_XML, this->model->certificate, this->model->SP_XML);
		free(this->model->SI_XML);
		free(this->model->SP_XML);
		free(this->model->SV_XML);
		free(this->model->certificate);
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}
