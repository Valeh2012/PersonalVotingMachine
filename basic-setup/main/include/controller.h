#ifndef MAIN_INCLUDE_CONTROLLER_H_
#define MAIN_INCLUDE_CONTROLLER_H_

/**
 * @file controller.h
 * @brief definition of controllers.
 * */

#include <string>
#include <algorithm>
#include <vector>
#include "string.h"
#include "esp_log.h"
#include "mbedtls/base64.h"
#include "mbedtls/asn1.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/sha256.h"
#include "expat.h"
#include "esp_gatts_api.h"

#include "module.h"
#include "model.h"
#include "view.h"

/**
 * \class Controller
 *
 * \brief abstract class Controller. C in MVC,
 *
 * Controllers are bridge between views and models. They take data from model, modify it if required and pass it to the view to present to client.
 * It has one main method - index(), which is called when controller is reached.
 * However, depending on their complexity , controller may have other methods.
 *
 * */
class Controller {
protected:
	View * vw;  /**< associated view object*/

public:
	virtual void index() = 0;  /**< abstract method index */
	virtual ~Controller(){};   /**< Destructor */
};

/**
 * \class IndexController
 * \brief sample inherited controller
 * */
class IndexController : public Controller {
public:
	IndexController(); /**< constructor */
	void index();     /**< a method that only prints "Index ctrl"*/
};

/**
 * \class AuthorizationController
 * \brief responsible for logic behind mobilID authorization
 *
 *
 * */
class AuthorizationController : public Controller{
private:
	UserModel * model; /**< user model*/

	const char * TAG = "authController";
	const char* sni; /**< SNI for authorization */
public:
	AuthorizationController(BaseModel *model, const char* sni); /**< constructor */

	/**
	 *  @brief JSON body generator to retrieve Authentication Certificate
	 *  Example:
	 *
	 *      {
	 *      	"id": 0.0,
	 *      	"method": "RPC.Authenticate",
	 *      	"params": [
	 *          	{
	 *					"OS": "Operating System,2,0",
	 *					"PhoneNo": "+37200000766",
	 *					"IdCode": "60001019906"
	 *				}
	 *		 	]
	 *	     }
	 *
	 * */
	char* generateAuthenticateRequestJSON();

	/**
	 *  @brief JSON body generator to retrieve Authentication Status
	 *
	 *  Example:
	 *
	 *  	{
	 *      	"id": 0.0,
	 *      	"method": "RPC.AuthenticateStatus",
	 *      	"params": [
	 *          	{
	 *			    	"OS": "Operating System,2,0",
	 *			        "SessionCode": "2127729011",
	 *			  	    "SessionID": "057229fdfa2df7d3c7f4ced81b02760b"
	 *			   	}
	 *		   	]
	 *     	}
	 *
	 * */
	char* generateAuthenticateStatusRequestJSON();

	/**
	 *  @brief index method
	 *
	 *  This method does nothing specific.
	 * */
	void index();

	/**
	 * @brief authorization request method.
	 *
	 * This method invokes authorization request to voting server.
	 * Server doesn't directly send authorization token. Instead, sends a challenge to user to verify (PIN code)
	 * Session ID is also returned which will be used in all upcoming requests.
	 * */
	void auth();

	/**
	 *  @brief authorization status method
	 *
	 *  After auth() called, server returns challenge code, session id and session code. Challenge code is displayed to user verify itself.
	 *  To check whether user has been verified and get authorization token that is also going to be used in all upcoming requests, authorization status
	 *  request has to be sent to server. This function sends that request until user verifies itself every 10 seconds.
	 *
	 *  Upon successful verification, authorization token is obtained and stored in model.
	 * */
	void authStatus();
};

/**
 *  \class ChoiceController
 *  \brief An authenticated user can request for list of candidates from voting server.
 *
 *  Voting server returns list of candidates in special scheme in JSON format. ChoiceController parses that response and
 *  make candidate names be displayed on screen.
 * */
class ChoiceController : public Controller{
private:
	ChoiceModel *model; /**< choices model */

	const char * TAG = "choicesCtrl";
	const char* sni; /**< SNI for choices service */
public:

	/**
	 *  @brief JSON body generator to retrieve the List of Choices
	 *
	 *  @param at Authentication token
	 *  @param ssid SessionID
	 *
	 *  Example:
	 *
	 *  	{
	 *     		"id": 0.0,
	 *     		"method": "RPC.VoterChoices",
	 *     		"params": [
	 *          	{
	 *              	"AuthMethod": "ticket",
	 *					"AuthToken": "G1RTZqBSBKrzqReuKYrmFUFXWFPvaxhJjdiZi6zqAnaK3OvrT...",
	 *					"OS": "Operating System,2,0",
	 *					"SessionID": "057229fdfa2df7d3c7f4ced81b02760b"
	 *				}
	 *	 		]
	 *		}
	 *
	 * */
	char* generateChoicesRequestJSON(ChoiceModel *cm);
	ChoiceController(BaseModel *model, const char* sni); /**< constructor*/

	/**
	 *  @brief fetches data from server and waits user select a candidate
	 * */
	void index();

	/**
	 *  Recursive function to parse JSON which holds the list of choices into choiceList,choiceListKeys and partyNameOf
	 *  @param item pointer to cJSON structure
	 *  @param par  party (also, parent) name of current element, otherwise ""
	 *
	 *  Example JSON body:
	 *
	 *  	{
	 *      	"Kuused":{
	 *          	"0000.101":"ARA SMIRNOVVVK",
	 *           	"0000.102":"MARGUS OTT KOVTP"
	 *        	},
	 *        	"Männid":{
	 *           	"0000.103":"ADINE SÄDEVVK"
	 *        	},
	 *        	"Üksikkandidaadid":{
	 *          	"0000.104":"LEILI KOVTP",
	 *           	"0000.107":"JUTA KOVVALAH",
	 *          	"0000.105":"KAJA KOVVALAG",
	 *           	"0000.106":"MAREK KOVVALAG"
	 *        	}
	 *     	}
	 *
	 * */
	void parse_object(cJSON *item, char* par);
};

/**
 * \class EncryptionController
 * \brief A controller where selected candidate is turned into ballot and encrypted
 *
 * */
class EncryptionController : public Controller{
private:
	EncryptionModel *model;  /**< encryption model*/
	const char * TAG = "encCtrl";
public:

	/**
	 *	@brief main controller method.
	 *
	 *	Encrypt voters casted ballot using public election key
	 *
	 * */
	void index();
	EncryptionController(BaseModel *model); /**< constructor 	*/
};



/**
 * \class SignatureController
 * \brief digitally sign encrypted ballot
 * */
class SignatureController : Controller{
private:
	SignatureModel *model;  /**< signature model*/
	const char* sni; /**< sni for signature service*/
	const char* TAG = "signCtrl";

	/**
	 * XML container for element SignedProperties
	 * */
	const char * SignedProperties = ""
			"<xades:SignedProperties xmlns:asic=\"http://uri.etsi.org/02918/v1.2.1#\" xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" xmlns:xades=\"http://uri.etsi.org/01903/v1.3.2#\" Id=\"S0-SignedProperties\">"
			"<xades:SignedSignatureProperties>"
			"<xades:SigningTime>%s</xades:SigningTime>"
			"<xades:SigningCertificate>"
			"<xades:Cert>"
			"<xades:CertDigest>"
			"<ds:DigestMethod Algorithm=\"http://www.w3.org/2001/04/xmlenc#sha256\"></ds:DigestMethod>"
			"<ds:DigestValue>%s</ds:DigestValue>"
			"</xades:CertDigest>"
			"<xades:IssuerSerial>"
			"<ds:X509IssuerName>%s</ds:X509IssuerName>"
			"<ds:X509SerialNumber>%s</ds:X509SerialNumber>"
			"</xades:IssuerSerial>"
			"</xades:Cert>"
			"</xades:SigningCertificate>"
			"</xades:SignedSignatureProperties>"
			"<xades:SignedDataObjectProperties>"
			"<xades:DataObjectFormat ObjectReference=\"#S0-RefId0\">"
			"<xades:MimeType>application/octet-stream</xades:MimeType>"
			"</xades:DataObjectFormat>"
			"</xades:SignedDataObjectProperties>"
			"</xades:SignedProperties>";

	/**
	 *  XML container for element SignedInfo
	 * */
	const char * SignedInfo = ""
			"<ds:SignedInfo xmlns:asic=\"http://uri.etsi.org/02918/v1.2.1#\" xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" xmlns:xades=\"http://uri.etsi.org/01903/v1.3.2#\">"
			"<ds:CanonicalizationMethod Algorithm=\"http://www.w3.org/2006/12/xml-c14n11\"></ds:CanonicalizationMethod>"
			"<ds:SignatureMethod Algorithm=\"http://www.w3.org/2001/04/xmldsig-more#ecdsa-sha256\"></ds:SignatureMethod>"
			"<ds:Reference Id=\"S0-RefId0\" URI=\"%s\">"
			"<ds:DigestMethod Algorithm=\"http://www.w3.org/2001/04/xmlenc#sha256\"></ds:DigestMethod>"
			"<ds:DigestValue>%s</ds:DigestValue>"
			"</ds:Reference>"
			"<ds:Reference Id=\"S0-RefId1\" Type=\"http://uri.etsi.org/01903#SignedProperties\" URI=\"#S0-SignedProperties\">"
			"<ds:Transforms>"
			"<ds:Transform Algorithm=\"http://www.w3.org/2006/12/xml-c14n11\"></ds:Transform>"
			"</ds:Transforms>"
			"<ds:DigestMethod Algorithm=\"http://www.w3.org/2001/04/xmlenc#sha256\"></ds:DigestMethod>"
			"<ds:DigestValue>%s</ds:DigestValue>"
			"</ds:Reference>"
			"</ds:SignedInfo>";

	/**
	 *  XML container for element SignatureValue
	 * */
	const char * SignatureValue = "<ds:SignatureValue xmlns:asic=\"http://uri.etsi.org/02918/v1.2.1#\" "
			"xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" xmlns:xades=\"http://uri.etsi.org/01903/v1.3.2#\" Id=\"S0-SIG\">%s</ds:SignatureValue>";

	/**
	 *  XML container for element XadESSignature
	 * */
	const char * Signature = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
			"<asic:XAdESSignatures xmlns:asic=\"http://uri.etsi.org/02918/v1.2.1#\">"
			"<ds:Signature xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" Id=\"S0\">"
			"%s"
			"%s"
			"<ds:KeyInfo>"
			"<ds:X509Data>"
			"<ds:X509Certificate>%s</ds:X509Certificate>"
			"</ds:X509Data>"
			"</ds:KeyInfo>"
			"<ds:Object>"
			"<xades:QualifyingProperties xmlns:xades=\"http://uri.etsi.org/01903/v1.3.2#\" Target=\"#S0\">"
			"%s"
			"</xades:QualifyingProperties>"
			"</ds:Object>"
			"</ds:Signature>"
			"</asic:XAdESSignatures>";

public:
	SignatureController(BaseModel *model, const char* sni);

	/**
	 * @brief retrieve user public key
	 * */
	void index();

	/**
	 *  @brief JSON body generator to get signing Certificate
	 *
	 *  Example:
	 *
	 *  	{
	 *      	"id": 0.0,
	 *     		"method": "RPC.GetCertificate",
	 *      	"params": [
	 *          	{
	 *					"AuthMethod": "ticket",
	 *					"AuthToken": "G1RTZqBSBKrzqReuKYrmFUFXWFPvaxhJjdiZi6zqAnaK3OvrT...",
	 *					"OS": "Operating System,2,0",
	 *					"SessionID": "057229fdfa2df7d3c7f4ced81b02760b"
	 *	 				"PhoneNo": "+37200000766",
	 *					"IdCode": "60001019906"
	 *				}
 	 *			]
	 *		}
	 *
	 * */
	char* generateGetCertificateRequestJSON();

	/*
	 *  JSON body generator to initiate vote signing

	 *  Example:
	 *		{
	 *			"id": 0.0,
	 *		  	"method": "RPC.Sign",
	 *		  	"params": [
	 *				{
	 *					"AuthMethod": "ticket",
	 *					"AuthToken": "G1RTZqBSBKrzqReuKYrmFUFXWFPvaxhJjdiZi6zqAnaK3OvrT...",
	 *					"Hash": "9IBrA05ylt2StdjxKkSTYMW/rQXY3Vub4upzShdfEzo=",
	 * 					"OS": "Operating System,2,0",
	 *					"SessionID": "057229fdfa2df7d3c7f4ced81b02760b"
	 *					"PhoneNo": "+37200000766",
	 *					"IdCode": "60001019906"
	 *				}
	 *			]
	 *		}
	 *
	 * */
	char* generateSignRequestJSON();

	/**
	 * @brief send sign request for encrypted ballot hash
	 *
	 *
	 * */
	void sign();

	/*
	 *  @brief JSON body generator to retrieve Signing Status
	 *
	 *  Example:
	 *
	 *  	{
	 *     		"id": 0.0,
	 *     		"method": "RPC.SignStatus",
	 *     		"params": [
	 *          	{
	 *					"OS": "Operating System,2,0",
	 *					"SessionCode": "2127729011",
	 *					"SessionID": "057229fdfa2df7d3c7f4ced81b02760b"
	 *				}
	 *			]
	 *		}
	 *
	 * */
	char* generateSignStatusRequestJSON();

	/**
	 * @brief check status of sign request
	 * */
	void status();

	/**
	 * @brief a helper method to put all xml elements into one single large elements.
	 * */
	void combine();
};

/**
 * \class ZipController
 * \brief BDOC container creator for singature file
 * */
class ZipController : public Controller{
private:
	ZipModel *model;

	const char* TAG = "zipCtrl";

	/**
	 * XML container for manifest.xml
	 */
	const char * manifest = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
			"<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\">\n"
			"  <manifest:file-entry manifest:full-path=\"/\" manifest:media-type=\"application/vnd.etsi.asic-e+zip\"/>\n"
			"  <manifest:file-entry manifest:full-path=\"%s\" manifest:media-type=\"application/octet-stream\"/>\n"
			"</manifest:manifest>";

public:
	ZipController(BaseModel *model); /**< constructor */

	/**
	 *  @brief create signature container according to BDOC 2.0 standards
	 *
	 *  Using slightly modified miniz library, create .zip archieve with following file structure
	 *
	 *  	\META-INF
	 *  		singature0.xml
	 *  		manifest.xml
	 *  	EP2065.1.ballot
	 *  	mimetype
	 *
	 */
	void index();
};

/**
 * \class VoteController
 * \brief cast signed ballot file to voting service
 *
 * */
class VoteController : public Controller{
private:
	VoteModel *model;

	const char * TAG = "voteCtrl";
	const char * sni;
public:
	VoteController(BaseModel *model, const char* sni); /**< constructor */

	/**
	 * @brief send bdoc container to voting service to cast your vote.
	 *
	 * A vote id is returned after bdoc container and vote data is validated in server side
	 * */
	void index();

	/**
	 * 	@brief JSON body generator to send signed vote
	 *
	 * 	Example
	 *
	 * 		{
	 *			"id": 0.0,
	 *			"method": "RPC.Vote",
	 *			"params": [
	 *				{
	 *					"AuthMethod": "ticket",
	 *					"AuthToken": "G1RTZqBSBKrzqReuKYrmFUFXWFPvaxhJjdiZi6zqAnaK3OvrT...",
	 *					"Choices": "0140.1",
	 *					"SessionID": "ec3a0cab353d552952289f2c7ad52e27",
	 *					"OS": "Operating System,2,0",
	 *					"Type": "bdoc",
	 *					"Vote": "UEsDBAoABgAAAAIAAAAbWltZXR5cGVhcHBsaWNhdGlv\nbi92bmQuZX..."
	 *				}
	 *			]
	 * 		}
	 * */
	char* generateVoteRequestJSON();
};

/**
 * \class BluetoothController
 * \brief send vote data to verification app via BLE
 *
 * */
class BluetoothController : public Controller{
private:
	BluetoothModel *model;  /**< bluetooth model */

	const char* TAG = "btCtrl";
public:
	BluetoothController(BaseModel* model);  /**< controller */

	/**
	 * @brief turn on BLE and send session id, randomness and vote id over BLE channel
	 * */
	void index();
};

#endif /* MAIN_INCLUDE_CONTROLLER_H_ */
