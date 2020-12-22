/**
 * @file QRController.cpp
 * @brief QRController implementation file
 * */

#include "qrcode.h"
#include "controller.h"

QRController::QRController(BaseModel *model){

	this->model = static_cast<QRModel *>(model);
	this->vw = new QRView();
}

void QRController::index(){

	// The structure to manage the QR code
	QRCode qrcode;

	// Allocate a chunk of memory to store the QR code
	uint8_t qrcodeBytes[qrcode_getBufferSize(23)];

	size_t len = strlen(this->model->ssid) + strlen(this->model->voteID) + strlen((char *)this->model->rndBase64) + 3;
	char* data = (char *) malloc(len);
	snprintf(data, len, "%s\n%s\n%s",this->model->ssid,this->model->rndBase64,this->model->voteID);  //! * concatenate fields

	// create qr code
	qrcode_initText(&qrcode, qrcodeBytes, 23, ECC_LOW, data);

	this->vw->render((void *) &qrcode);
}
