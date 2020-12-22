#ifndef MAIN_INCLUDE_CONVERTER_H_
#define MAIN_INCLUDE_CONVERTER_H_

/**
 *  @file converter.h
 *  @brief converter namespace - collection of frequently used conversion/encoding/decoding functions
 *
 * */

#include <stdint.h>
#include <string.h>
#include "expat.h"

/** Buffer size for xml parsing */
#define output_size 10240

/**
 * \namespace converter
 * */
namespace converter{

/** @brief a helper function to convert pem files to der.
 *
 * source: https://github.com/ARMmbed/mbedtls/blob/master/programs/util/pem2der.c
 *
 * @param input pem file buffer
 * @param ilen  input buffer length
 * @param output buffer to store converted der content
 * @param olen length of data written in output buffer
 *
 * @returns 0 if OK, else -1
 * */
int convert_pem_to_der( const unsigned char *input, size_t ilen,
                        unsigned char *output, size_t *olen );

/**
 * @brief a helper function to convert long hex numbers to base b = 2^32
 *
 * In base 2^32, every digit is 32 bit length unsigned integers (uint32_t).
 * Therefore, each consequent 4 bytes in hex converted into one digit in base b.
 * Note: numbers may contain '0x00' in hex representation which is treated as end of string
 * in string functions. Another parameter is required to indicate length of hex representation
 *
 * @param hex input number buffer
 * @param len input buffer length
 *
 * @returns long base-b number
 */
uint32_t* hex2u32(unsigned char* hex, size_t len);

/**
 * @brief a helper function to convert long base-b number to hex values for DER encoding
 *
 * Inverse of hex2u32 function
 *
 * @param x long base-b number
 * @param length length of input number
 * @param len output buffer length
 *
 * @returns buffer containing number in hex representation
 * */
unsigned char * nb(uint32_t *x, int length, size_t *olen);

/**
 * @brief a helper function convert to first hash, then base64 encode
 *
 * This function wraps to mbed_tls functions. Given input string buffer,
 * it first finds SHA256 hash, then converts the resulting hash into base64.
 *
 * @param input string buffer
 * @param len input buffer length
 *
 * @returns output buffer
 * */
unsigned char * base64hash(unsigned char* input, size_t len);

/** @brief struct used in xml parsing functions
 * */
typedef struct {
    int depth;  /**< depth of recursion*/
    char* output; /**< output buffer (for parsed xml) */
    int output_off; /**< output buffer offset (last written index, i.e length of written data)*/
    const char *last_name; /**< last opened xml tag */
} xml_data_t;

/**
 *  @brief a helper function to insert space for indentation
 *
 *  This function prints extra whitespaces to output buffer for indentation purposes.
 *
 *  @param user_data parsed xml data
 * */
static void insert_space(xml_data_t *user_data);

/**
 *  @brief a callback function called at start of new xml node
 *
 *  XMLParser traverses through xml nodes and each time it reaches a new node, this callback function is called.
 *  Copy tag name and attributes to output buffer
 *
 *  @param user_data parsed xml data
 *  @param name just opened current xml node tag name
 *  @param atts other attributes of current xml node
 * */
static void XMLCALL start_element(void *userData, const XML_Char *name, const XML_Char **atts);

/**
 *  @brief a callback function called at the end of new xml node
 *
 *  XMLParser traverses through xml nodes and each time it reaches a closing tags of current node, this callback function is called.
 *  If current node is leaf node, add closing tags immediately. Otherwise, add whitespace before closing tags.
 *
 *  @param user_data parsed xml data
 *  @param name to be closed current xml node tag name
 * */
static void XMLCALL end_element(void *userData, const XML_Char *name);

/**
 *  @brief a function to define how to parse xml elements
 *
 *  In between starting and closing tags, this functions is called by XMLParser.
 *  Here, it just copies data buffer to output buffer without adding extra whitespace.
 *
 *  @param user_data parsed xml data
 *  @param s data in current xml node
 *  @param len length of data
 * */
static void data_handler(void *userData, const XML_Char *s, int len);

/**
 * @brief a helper funciton to canonicalize xml data (XMLC14N11).
 *
 * Specifications can be found here: \see https://www.w3.org/TR/xml-c14n11/#Example-SETags
 *
 * @param str xml input buffer
 * @param len input buffer length
 * */
void xmlc14n11(char* str, size_t len);
}

#endif
