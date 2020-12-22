/**
 * @file RPCModule.cpp
 * @brief RPC implementation file
 * */

#include "string.h"
#include "module.h"

RPC::RPC(){

	this->cfg = new esp_tls_cfg_t();
	this->server = NULL;
	this->port = 443;
};

cJSON * RPC::send_json_rpc(const char* sni, char* req){

	static char buf[512]; // tls connection buffer to read response
	int length = 1024;
	char *res;   // buffer to store whole response body
	res = (char *) malloc(length);
	memset(res, 0, length);
	int ret, len;

	/* tls connection config.  */
	cfg->common_name = sni;  //! set SNI extension

	struct esp_tls *tls = esp_tls_conn_new(server, strlen(server),port, cfg); //! connect to defined address

	if(tls != NULL){
		ESP_LOGI(TLS_TAG, "Connection established with %s", sni);
	} else {
		printf("available memory: %d\n", (int) heap_caps_get_free_size(MALLOC_CAP_32BIT | MALLOC_CAP_8BIT));
		ESP_LOGE(TLS_TAG, "Connection failed...");
		throw "Connection failed";
	}

	size_t written_bytes = 0;
	do {
		ret = esp_tls_conn_write(tls, req + written_bytes, strlen(req) - written_bytes);  //! send rpc data

		if(ret >= 0){
			written_bytes += ret;
		} else if (ret != MBEDTLS_ERR_SSL_WANT_READ  && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			ESP_LOGE(TLS_TAG, "esp_tls_conn_write  returned 0x%x", ret);
			esp_tls_conn_delete(tls);
		}
	} while(written_bytes < strlen(req));

	do{
		len = sizeof(buf) - 1;
		bzero(buf, sizeof(buf));
		ret = esp_tls_conn_read(tls, (char *)buf, len);  //! recieve response in chunks
		if(ret == MBEDTLS_ERR_SSL_WANT_WRITE  || ret == MBEDTLS_ERR_SSL_WANT_READ)
			continue;

		if(ret < 0){
			ESP_LOGE(TLS_TAG, "esp_tls_conn_read  returned -0x%x", -ret);
			break;
		}

		if(ret == 0){
			ESP_LOGI(TLS_TAG, "connection closed");
			break;
		}

		len = ret;
		ESP_LOGD(TLS_TAG, "%d bytes read", len);

		if(strlen(res) + len + 1 >= length){  //! In case no enough memory, extend res buffer
			length += len;                    //! * increase by amount required
			res = (char*) realloc(res, 	length);      //! * reallocate memory
			memset(res+length-len,0,len);     //! * set new memory blocks 0 to assure the result is null-terminate string
		}
		memcpy(res + strlen(res), buf, len); //! write chunked response to buffer

	} while(1);

	cJSON *tmp = cJSON_Parse(res);   //! parse response JSON into cJSON struct
	free(res);
	free(req);
	esp_tls_conn_delete(tls);
	return tmp;
}

RPC& RPC::Instance(){

	static RPC instance;
	return instance;
}
