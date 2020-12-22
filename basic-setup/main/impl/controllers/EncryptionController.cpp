/**
 * @file EncryptionController.cpp
 * @brief EncryptionController implementation file
 * */

#include "esp_log.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
#include "mbedtls/bignum.h"
}

#include "controller.h"
#include "converter.h"

#ifndef modlength
#define modlength 96  /**< 3072 bits = 384 bytes = 96 words */
#endif

/**
 * Struct to store ElGamal encryption results.
 * g is group generator and pk is public key
 * In general, ElGamal encyption cipher is a pair (c1, c2). But, we'll need randomness later for verification,
 * so we also keep random number
 * */
typedef struct{
	mbedtls_mpi c1; /**< c1 = g^r mod p */
	mbedtls_mpi c2; /**< c2 = c1*g^pk mod p */
	mbedtls_mpi random; /**< r */
}elgamal_cipher_t;

typedef struct{
	mbedtls_mpi* base;
	mbedtls_mpi* exponent;
	mbedtls_mpi* buff;
	mbedtls_mpi* mod;
	uint8_t flag;
	int error;
}task_param_t;

void taskMont(void* data){

	task_param_t* params = (task_param_t*) data;

	params->error = mbedtls_mpi_exp_mod(params->buff, params->base, params->exponent, params->mod, NULL);

	params->flag = 1;

	for(;;){

	}
}

/**
 * @brief secure big random generator
 *
 * Multi-precision integer (mpi) is an array of 64bit unsigned integers in big endian representation. This method
 * creates grows input parameter to the length of modlength, then fills the array with random unsigned integers using
 * esp_random()
 *
 * @param mpi The destination MPI. This must point to an initialized MPI.
 *
 */
void mpi_RandomNumber(mbedtls_mpi * mpi ){

	srand ( time(NULL) );
	mbedtls_mpi_grow(mpi, modlength);

	for(int i = 0;i < modlength; i++){
	  mpi->p[i] = (mbedtls_mpi_uint) esp_random();
	}
}

/**
 *   @brief fast multi-core ElGamal implementation
 *   ElGamal is a homomorphic encryption method used in ivxv protocol.
 *   3072 bit long keys provide 128 bit security.
 *
 *   @param pk public key
 *   @param m  encoded plaintext
 *   @param res pointer to cipher struct
 *	 @param mod pointer to modulus variable
 *
 *   @returns 0 if successful, -1 otherwise
 * */
int ElGamalEncrypt(mbedtls_mpi * pk, mbedtls_mpi* m, elgamal_cipher_t * res, mbedtls_mpi * mod){

	esp_err_t error;
	mbedtls_mpi rnd, g, c1,  c2;
	mbedtls_mpi_init(&c1);
	mbedtls_mpi_init(&c2);
	mbedtls_mpi_init(&rnd);
	mbedtls_mpi_init(&g);

	mpi_RandomNumber(&rnd);
	error = mbedtls_mpi_read_string(&g, 10, "2");
	if(error) return error;

	task_param_t * c1Params = new task_param_t();
	c1Params->base = &g;
	c1Params->exponent = &rnd;
	c1Params->buff = &c1;
	c1Params->mod = mod;

	TaskHandle_t c1Task;
	xTaskCreatePinnedToCore(taskMont, "C1_Mont", 4192, (void *) c1Params, 0 , &c1Task, 1);

	mbedtls_mpi c3;
	mbedtls_mpi_init(&c3);
	error = mbedtls_mpi_exp_mod(&c3, pk, &rnd, mod, NULL);
	if(error) return error;

	error = esp_mpi_mul_mpi_mod(&c2, &c3, m, mod);
	if(error) return error;

	while(!c1Params->flag){

	}
	vTaskDelete(c1Task);
	if(c1Params->error) return c1Params->error;

	error = mbedtls_mpi_copy(&res->c1, &c1); if(error) return error;
	error = mbedtls_mpi_copy(&res->c2, &c2); if(error) return error;
	error = mbedtls_mpi_copy(&res->random, &rnd); if(error) return error;

	mbedtls_mpi_free(&c1);
	mbedtls_mpi_free(&c2);
	mbedtls_mpi_free(&c3);
	mbedtls_mpi_free(&rnd);
	mbedtls_mpi_free(&g);

	return 0;
}

EncryptionController::EncryptionController(BaseModel *model){

	this->model = static_cast<EncryptionModel *>(model);
	this->vw = new IndexView();
}

void EncryptionController::index(){

	try{
		this->vw->render((void *)"Encrypting ballot...");

		unsigned char * PK_DER = (unsigned char *) malloc(1024);
		memset(PK_DER, 0, 1024);
		size_t lenPK = 1024;
		memset(PK_DER, 0 , lenPK);
		int error = converter::convert_pem_to_der(this->model->keypem, this->model->keypem_length, PK_DER, &lenPK);  //! * First, convert election public key file to DER format*/
		if(error) throw "unable to parse pem file";

		unsigned char *p = PK_DER;
		const unsigned char *end = p + lenPK;
		size_t len; int i;

		mbedtls_asn1_buf  oid, oid_params;
		memset( &oid, 0, sizeof( mbedtls_asn1_buf ) );
		memset( &oid_params, 0, sizeof( mbedtls_asn1_buf ) );

		mbedtls_mpi mod;
		mbedtls_mpi_init(&mod);

		//! * Start decoding ASN1 DER encoded key contents using mbedtls_asn1 library /
		error = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);  if(error) throw "ASN1 parse error";
		error = mbedtls_asn1_get_alg(&p, end, &oid, &oid_params); if(error) throw "ASN1 parse error";
		unsigned char * q = oid_params.p;
		const unsigned char *q_end = q + oid_params.len;
		error = mbedtls_asn1_get_mpi(&q, q_end, &mod); if(error) throw "ASN1 parse error"; //! * extract mod

		error = mbedtls_asn1_get_tag(&q, q_end, &len, MBEDTLS_ASN1_INTEGER); if(error) throw "ASN1 parse error";
		q += len;
		error = mbedtls_asn1_get_tag(&q, q_end, &len, 0x1B );  if(error) throw "ASN1 parse error";    //! * extract electionID
		this->model->election_id = (char *)malloc(len+1);
		memset(this->model->election_id, 0, len+1);
		memcpy(this->model->election_id, q, len);

		mbedtls_asn1_bitstring bs = {0, 0 , NULL};
		error = mbedtls_asn1_get_bitstring(&p, end, &bs); if(error) throw "ASN1 parse error";
		p = bs.p;
		error = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE); if(error) throw "ASN1 parse error";

		mbedtls_mpi key;
		mbedtls_mpi_init(&key);

		error = mbedtls_asn1_get_mpi(&p, end, &key); if(error) throw "ASN1 parse error"; //! * extract 3072bit public key
		if(error) throw "key error";
		if(p != end){
			printf("Error. Didn't parse until the end\n");
			throw "Incomplete ASN";
		}

		/**
		 * * Then, pad the ballot using following scheme
		 *
		 *   		padded-ballot = '\x00' '\x01' *'\xff' '\x00' ballot
		 *
		 */
		len = mbedtls_mpi_bitlen(&key);
		len /= 8;
		unsigned char *padded_ballot = (unsigned char *) malloc(len + 1);
		memset(padded_ballot, 0xFF, len);
		padded_ballot[0] = 0x00;
		padded_ballot[1] = 0x01;
		padded_ballot[len] = 0x00;  // end of string (null-terminated)
		padded_ballot[len - strlen(this->model->ballot) - 1] = 0x00;
		for(i=1;i<=strlen(this->model->ballot);i++){
			padded_ballot[len - i] = this->model->ballot[strlen(this->model->ballot) - i];
		}

		mbedtls_mpi plaintext;
		mbedtls_mpi_init(&plaintext);

		error = mbedtls_mpi_read_binary(&plaintext, padded_ballot, len); //! * convert padded ballot to group element
		if (error) throw "ballot mpi conversion error";
		free(padded_ballot);

		mbedtls_mpi pq;  //pq = (modulus-1)/2
		mbedtls_mpi_init(&pq);
		error = mbedtls_mpi_sub_int(&pq, &mod, 1); if(error) throw "error at mod - 1";
		error = mbedtls_mpi_div_int(&pq, NULL, &pq, 2); if(error) throw "error at pq // 2";

		//! * encode plaintext as a quadratic residue in the set
		mbedtls_mpi encoded_plaintext, res;
		mbedtls_mpi_init(&encoded_plaintext);
		mbedtls_mpi_init(&res);
		error = mbedtls_mpi_exp_mod(&res, &plaintext, &pq, &mod, NULL); if(error) throw "quadratic residue check error";
		if(	mbedtls_mpi_cmp_int(&res,  1) == 0){
			error = mbedtls_mpi_copy(&encoded_plaintext, &plaintext);
			if(error) throw "mpi copy error";
			ESP_LOGI(TAG, "quadratic residue");
		}else{
			error = mbedtls_mpi_sub_mpi(&encoded_plaintext, &mod, &plaintext);
			if(error) throw "mpi substract error";
		}

		elgamal_cipher_t *cipher = new elgamal_cipher_t();
		mbedtls_mpi_init(&cipher->c1);
		mbedtls_mpi_init(&cipher->c2);
		mbedtls_mpi_init(&cipher->random);

		time_t before_enc, after_enc;
		time(&before_enc);
		error = ElGamalEncrypt(&key, &encoded_plaintext, cipher, &mod); //! * encrypt and store results in cipher
		time(&after_enc);
		ESP_LOGI(TAG, "encryption took %ld seconds", (long) after_enc - (long) before_enc);
		if(error) throw "Encryption error";
		ESP_LOGI(TAG, "plaintext encrypted");

		unsigned char * rndASN = (unsigned char*) malloc( len );
		memset(rndASN, 0, len);
		error = mbedtls_mpi_write_binary(&cipher->random, rndASN, len );  //! * convert used random number into hex and base64 encode it.
		if(error) throw "error in rnd binary export";

		this->model->rndBase64 = (unsigned char *)malloc(2*len);
		memset(this->model->rndBase64,0, 2*len);
		error = mbedtls_base64_encode(this->model->rndBase64, 2*len, &len, rndASN, len);
		if(error) throw "Random number hash error";
		//! * start DER encoding of cipher
		puts((char *) this->model->rndBase64);

		unsigned char * asnder = (unsigned char *) malloc(1220);
		memset(asnder, 0, 1220);
		unsigned char *asn_end = asnder + 1220;
		int succ, total_succ = 0;

		succ = mbedtls_asn1_write_mpi(&asn_end, asnder, &cipher->c2); //! * convert second component of cipher into hex and encode as ASN1 Integer
		total_succ+=succ;

		succ = mbedtls_asn1_write_mpi(&asn_end, asnder, &cipher->c1); //! * convert first component of cipher into hex and encode as ASN1 Integer
		total_succ+=succ;

		succ = mbedtls_asn1_write_len(&asn_end, asnder, total_succ);
		total_succ+=succ;
		succ = mbedtls_asn1_write_tag(&asn_end, asnder, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
		total_succ+=succ;

		int params_total = 0;
		succ = mbedtls_asn1_write_raw_buffer(&asn_end, asnder, oid_params.p, oid_params.len);
		params_total+=succ;
		succ = mbedtls_asn1_write_len(&asn_end, asnder, oid_params.len);
		params_total+=succ;
		succ = mbedtls_asn1_write_tag(&asn_end, asnder, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
		params_total+=succ;

		succ = mbedtls_asn1_write_algorithm_identifier(&asn_end, asnder, (char *) oid.p, oid.len, params_total); //! * append encryption algorithm identifier to DER
		total_succ+=succ;

		succ = mbedtls_asn1_write_len(&asn_end, asnder, total_succ);
		total_succ+=succ;
		succ = mbedtls_asn1_write_tag(&asn_end, asnder, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
		total_succ+=succ;

		this->model->ballotASN = (unsigned char *) malloc(total_succ+1);
		memset(this->model->ballotASN, 0, total_succ + 1);
		memcpy(this->model->ballotASN, asnder+1220-total_succ, total_succ);
		this->model->ballotLength = total_succ;

		free(asnder);
		free(PK_DER);
		free(rndASN);
		mbedtls_mpi_free(&mod);
		mbedtls_mpi_free(&pq);
		mbedtls_mpi_free(&key);
		mbedtls_mpi_free(&plaintext);
		mbedtls_mpi_free(&encoded_plaintext);
		mbedtls_mpi_free(&res);
		mbedtls_mpi_free(&cipher->c1);
		mbedtls_mpi_free(&cipher->c2);
		mbedtls_mpi_free(&cipher->random);

		// SHA256 hash + Base64 asnder
		this->model->ballotFileName = (char *)malloc(65);
		memset(this->model->ballotFileName, 0, 65);
		snprintf(this->model->ballotFileName, 64, "%s.%s.ballot", this->model->election_id, "1");  //! * set filename for DER encoded encrypted ballot

		this->model->ballotHash = converter::base64hash(this->model->ballotASN, this->model->ballotLength); //! * hash and base64 encode it for future references

		ESP_LOGI(TAG, "ballot hash: %s\n",(char *)this->model->ballotHash);

		this->vw->render((void*) "Encrypted");
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}
