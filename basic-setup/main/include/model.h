#ifndef MAIN_INCLUDE_MODEL_H_
#define MAIN_INCLUDE_MODEL_H_

#include <vector>
#include "stdint.h"

/**
 * @file model.h
 * @brief header for model definitions
 * */


/**
 * \class BaseModel
 * \brief base model class
 * */
class BaseModel{
public:
	char* ssid;  /**< session ID*/
	char* authToken; /**< authentication token */
};

/**
 * \class UserModel
 * \brief data model to use in AuthorizationController
 * */
class UserModel : public BaseModel{
public:
	char ID[12];  /**< ID code of user */
	char phone[13];  /**< Phone number of user */
	char* sscode;   /**< session code for PIN confirmation */

	UserModel(char* id, char* phone);  /**< constructor */
};

/**
 * @brief struct to store candidate details
 * */
typedef struct{
	char * party; /**< candidate party name */
	char * code;  /**< District code */
	char * candidate; /**< candidate full name */
}choice_t;

/**
 * \class ChoiceModel
 * \brief data model to use in ChoiceController
 * */
class ChoiceModel : public BaseModel{
public:
	std::vector<choice_t> *choice_list;   /**< list of candidates*/
	char* choices;  /**< The voter’s district identifier */
	char* ballot;  /**< ballot generated from selected candidate */

	ChoiceModel(char *ssid, char* authToken); /**< constructor */
};

/**
 * \class EncryptionModel
 * \brief data model to use in EncryptionController
 * */
class EncryptionModel : public BaseModel{
public:
	char* ballot;  /**< ballot generated from selected candidate */
	unsigned char* ballotASN; /**< encoded encrypted ballot ballot data in ASN1 DER */
	size_t ballotLength; /**< ballotASN file length */
	unsigned char* rndBase64; /**< Base64 encoded randomness used in ecryption */
	unsigned char* ballotHash; /**< ballotASN hash */
	char* ballotFileName; /**< filename for encrypted ballot */
	char* election_id; /**< election id from public election key*/
	const unsigned char* keypem;  /**< public election file in PEM  */
	int keypem_length; /**< public key buffer length */

	EncryptionModel(char* ballot,char* ssid, char * authToken, const uint8_t keypem[], int keypem_length); /**< constructor */
};


/**
 * \class SignatureModel
 * \brief data model to use in SignatureController
 * */
class SignatureModel : public BaseModel{
public:
	char ID[12];  /**< ID code of user */
	char phone[13];  /**< Phone number of user */
	char* sscode;   /**< session code for PIN confirmation */
	unsigned char* ballotHash; /**< ballotASN hash */
	size_t         ballotLength; /**< ballotASN file length */
	char*          ballotFileName; /**< filename for encrypted ballot */
	unsigned char* SignedInfoHash; /**< hash of SignedInfo element  */
	char* signature; /**< digital signature returned from mobilID*/
	char* certificate; /**< signature certificate */
	char* SI_XML;  /**< SignedInfo XML element*/
	char* SP_XML;  /**< SignedProperties XML element*/
	char* SV_XML;  /**< SignatureValue XML element*/
	char* Signature; /**< Signature XML element*/

	SignatureModel(char* ssid, char * authToken, char* phone, char* id, unsigned char* ballotHash, size_t ballotLength, char* ballotFileName);
};

/**
 * \class ZipModel
 * \brief data model to use in ZipController
 * */
class ZipModel : public BaseModel {
public:
	unsigned char* ballot;  /**< encrypted digital ballot */
	char* 	ballotFileName; /**< ballot file name */
	size_t  ballotLength;   /**< ballot length */
	char*   Signature;      /**< Signature XML*/
	unsigned char* voteBase64;  /**< Base64 encoded bdoc container*/

	ZipModel(unsigned char* ballot, char* ballotFileName, size_t ballotLength, char* Signature); /** constructor */
};

/**
 * \class VoteModel
 * \brief data model to use in VoteController
 * */
class VoteModel : public BaseModel{
public:
	unsigned char* voteBase64; /**< Base64 encoded bdoc container*/
	char* voteID;  /**< voteID */
	char* choices; /**< voters district identifier */

	VoteModel(char* ssid, char* authToken, unsigned char* voteBase64, char* choices); /**< constructor */
};

/**
 * \class BluetoothModel
 * \brief data model to use in BluetoothController
 * */
class BluetoothModel : public BaseModel{
public:
	char* ssid;  /**< session ID */
	char* voteID; /**< voteID */
	unsigned char* rndBase64; /**< Base64 encoded randomness used in encryption */

	BluetoothModel(char* ssid, char* voteID, unsigned char* rndBase64); /**< constructor */
};

#endif /* MAIN_INCLUDE_MODEL_H_ */
