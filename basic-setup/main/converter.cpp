
/**
 *  @file converter.cpp
 *
 *  @brief namespace converter implementation
 * */

#include <algorithm>
#include <vector>
#include "cJSON.h"
#include "esp_log.h"
#include "unity.h"
#include "mbedtls/base64.h"
#include "mbedtls/asn1.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/sha256.h"

#include "converter.h"

unsigned char * converter::base64hash(unsigned char* input, size_t len){

	unsigned char *hashBase64 = (unsigned char *) malloc(64);
	memset(hashBase64, 0, 64);

	static unsigned char hashResult[32];
	memset(hashResult, 0, 32);

	int error = mbedtls_sha256_ret(input, len, hashResult, 0);
	if(error) throw "Unable to hash";

	error = mbedtls_base64_encode(hashBase64, 64, &len, hashResult, 32);
	if(error) throw "Unable to base64 encode";

	return hashBase64;
}

// source: https://github.com/ARMmbed/mbedtls/blob/master/programs/util/pem2der.c
int converter::convert_pem_to_der( const unsigned char *input, size_t ilen,
        unsigned char *output, size_t *olen ){
	 int ret;
	    const unsigned char *s1, *s2, *end = input + ilen;
	    size_t len = 0;

	    s1 = (unsigned char *) strstr( (const char *) input, "-----BEGIN" );
	    if( s1 == NULL )
	        return( -1 );

	    s2 = (unsigned char *) strstr( (const char *) input, "-----END" );
	    if( s2 == NULL )
	        return( -1 );

	    s1 += 10;
	    while( s1 < end && *s1 != '-' )
	        s1++;
	    while( s1 < end && *s1 == '-' )
	        s1++;
	    if( *s1 == '\r' ) s1++;
	    if( *s1 == '\n' ) s1++;

	    if( s2 <= s1 || s2 > end )
	        return( -1 );

	    ret = mbedtls_base64_decode( NULL, 0, &len, (const unsigned char *) s1, s2 - s1 );
	    if( ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER )
	        return( ret );

	    if( len > *olen )
	        return( -1 );

	    if( ( ret = mbedtls_base64_decode( output, len, &len, (const unsigned char *) s1,
	                               s2 - s1 ) ) != 0 )
	    {
	        return( ret );
	    }

	    *olen = len;

	    return( 0 );
}

uint32_t* converter::hex2u32(unsigned char* hex, size_t len){

	static uint32_t result[96];
	int j = 0, i = len;
	while(j<  96 && i >0){  // group every 4 byte and cast to uint32_t, then append to array
		uint32_t hex3 = i > 2 ? hex[i-3] & 0xFF : 0;
		uint32_t hex2 = i > 1 ? hex[i-2] & 0xFF : 0;
		uint32_t hex1 = i > 0 ? hex[i-1] & 0xFF : 0;
		uint32_t hex0 = hex[i] & 0xFF;
		result[j] =  (hex3 << 24 ) | (hex2 << 16 ) | ( hex1 << 8) | hex0;  // because uint32_t is 4 bytes
		i= i - 4;
		j++;
	}

	return result;
}

unsigned char * converter::nb(uint32_t *x, int length, size_t *olen){

	unsigned char * s = (unsigned char *) malloc(length * sizeof(uint32_t));
    memset(s,0, length * 4);
    *olen = 0;
    for(int i=length-1;i>=0;i--){
    	unsigned char c1 = (unsigned char )  x[i] & 0xFF;
    	unsigned char c2 = (unsigned char )  (x[i] >> 8) & 0xFF;
    	unsigned char c3 = (unsigned char )  (x[i] >> 16) & 0xFF;
    	unsigned char c4 = (unsigned char )  (x[i] >> 24) & 0xFF;
    	memset(s+(length-1 - i)*4+0, c4, sizeof(c4));
    	memset(s+(length-1 - i)*4+1, c3, sizeof(c3));
    	memset(s+(length-1 - i)*4+2, c2, sizeof(c2));
    	memset(s+(length-1 - i)*4+3, c1, sizeof(c1));
    	*olen += 4;
    }

    if( (s[0] & 0xFF) >> 7 ){ // If first bit of first octet is 1, add zeros octet in-front
    	unsigned char * s2 = (unsigned char *) malloc(length * sizeof(uint32_t) + 1);
    	memset(s2,0,1);
    	memcpy(s2+1,s, *olen);
    	*olen +=1;
    	return s2;
    }
    return s;
}

void converter::insert_space(xml_data_t *user_data){

    const char align_str[] = "  ";

    TEST_ASSERT(output_size >= user_data->output_off);
    user_data->output[user_data->output_off++] = '\n';

    for (int i = 0; i < user_data->depth; i++) {
        for (int j = 0; j < strlen(align_str); ++j) {
            TEST_ASSERT(output_size >= user_data->output_off);
            user_data->output[user_data->output_off++] = align_str[j];
        }
    }
}

void XMLCALL converter::start_element(void *userData, const XML_Char *name, const XML_Char **atts){

	xml_data_t *user_data = (xml_data_t *) userData;
    if(user_data->depth)
    	insert_space(user_data);
    user_data->last_name = name;

    int ret;
    if(atts[0] == NULL){
    	ret = snprintf(user_data->output + user_data->output_off,
    			output_size - user_data->output_off, "<%s>", name);
    }
    else{
    	int i=0;
    	static char atts_imploded[512];
    	bzero(atts_imploded, 512);
		while(atts[i] != NULL){
			strncat(atts_imploded , atts[i], strlen(atts[i]));
			i++;
			if(atts[i]!= NULL){
				strncat(atts_imploded, "=\"", 4);
				strncat(atts_imploded, atts[i], strlen(atts[i]));
				strncat(atts_imploded, "\" ", 4);
				i++;
			}

		}

		atts_imploded[strlen(atts_imploded)-1] = '\0';
		ret = snprintf(user_data->output + user_data->output_off,
				output_size - user_data->output_off,
		            "<%s %s>", name, atts_imploded);
    }

    user_data->output_off += ret;
    ++user_data->depth;
}

void XMLCALL converter::end_element(void *userData, const XML_Char *name){

    xml_data_t *user_data = (xml_data_t *) userData;

    --user_data->depth;
    if(strcmp(user_data->last_name, name)) // do not add extra space in between one line xml tags
    	insert_space(user_data);

    int ret = snprintf(user_data->output + user_data->output_off, output_size - user_data->output_off,
                "</%s>", name);
    TEST_ASSERT_EQUAL(strlen(name) + 3, ret); // 3 are the tag characters: "</>"
    user_data->output_off += ret;
}

void converter::data_handler(void *userData, const XML_Char *s, int len){

	xml_data_t *user_data = (xml_data_t *) userData;

    // s is not zero-terminated
    char tmp_str[len+1];
    memset(tmp_str, 0, len+1);
    strncpy(tmp_str, s, len);
    int ret = snprintf(user_data->output + user_data->output_off, output_size - user_data->output_off,
                "%s", tmp_str);
    TEST_ASSERT_EQUAL(strlen(tmp_str), ret);
    user_data->output_off += ret;
}

void converter::xmlc14n11(char* str, size_t len){

	xml_data_t *user_data = new xml_data_t();

	user_data->depth = 0;
	user_data->output =  (char *) malloc(output_size);
	user_data->output_off = 0;
	user_data->last_name = {};

	TEST_ASSERT_NOT_NULL(user_data->output);

	memset(user_data->output, 0, output_size);

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, user_data);
	XML_SetElementHandler(parser, start_element, end_element);
	XML_SetCharacterDataHandler(parser, data_handler);

	TEST_ASSERT_NOT_EQUAL(XML_STATUS_ERROR, XML_Parse(parser, str, strlen(str), 1));
	XML_ParserFree(parser);

	TEST_ASSERT_EQUAL(0, user_data->depth); // all closing tags have been found

	memset(str, 0, len);
	memcpy(str, user_data->output, user_data->output_off);

	free(user_data->output);
	delete(user_data);
}
