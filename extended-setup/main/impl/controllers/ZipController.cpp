/**
 * @file ZipController.cpp
 * @brief ZipController implementation file
 * */

#include "miniz.h"
#include "controller.h"

ZipController::ZipController(BaseModel *model){

	this->vw = new IndexView();
	this->model = static_cast<ZipModel *>(model);
}

void ZipController::index(){

	try{
		this->vw->render((void *) "Sign" );
		this->vw->setLabel((char *) "Creating BDOC");
		this->vw->showLoader(true);
		// create a container with .ballot and .xml files
		mz_bool status;
		mz_zip_archive bdoc;
		mz_zip_zero_struct(&bdoc);
		size_t len;
		status = mz_zip_writer_init_heap(&bdoc, 0,10240);  //! * init zip heap
		if(!status){
		  throw mz_zip_get_error_string(bdoc.m_last_error);
		}

		// Add mimefile
		char *mimetype = (char *) "application/vnd.etsi.asic-e+zip";   //! * add mimetype file to zip
		status = mz_zip_writer_add_mem(&bdoc, "mimetype", (void *) mimetype, strlen(mimetype), MZ_NO_COMPRESSION);
		if (!status){
			throw mz_zip_get_error_string(bdoc.m_last_error);
		}

		// Add manifest.xml
		char* manifestXML = (char *) malloc(512);
		memset(manifestXML,0,512);
		snprintf(manifestXML, 512, manifest, "EP2065.1.ballot");

		status = mz_zip_writer_add_mem(&bdoc, "META-INF/manifest.xml", (void *)manifestXML, strlen(manifestXML), MZ_NO_COMPRESSION);  //! * add manifest.xml file to zip
		if (!status){
			throw mz_zip_get_error_string(bdoc.m_last_error);
		}

		// Add ballot file
		status = mz_zip_writer_add_mem(&bdoc, this->model->ballotFileName, (void *) this->model->ballot, this->model->ballotLength, MZ_NO_COMPRESSION);  //! * add ballot file to zip
		if (!status){
			throw mz_zip_get_error_string(bdoc.m_last_error);
		}

		// Add signature0.xml
		status = mz_zip_writer_add_mem(&bdoc, "META-INF/signatures0.xml", (void *) this->model->Signature, strlen(this->model->Signature), MZ_NO_COMPRESSION);   //! * add signature file to zip

		if (!status){
			throw mz_zip_get_error_string(bdoc.m_last_error);
		}
		// Close zip file
		void *zipBuf;
		status = mz_zip_writer_finalize_heap_archive(&bdoc, &zipBuf, &len);
		if (!status){
			throw mz_zip_get_error_string(bdoc.m_last_error);
		}

		status = mz_zip_writer_end(&bdoc);
		if (!status){
			throw "zip end failed";
		}

		// hash container
		this->model->voteBase64 = (unsigned char *)malloc(2*len);
		if(this->model->voteBase64 == NULL){
			throw "Insufficient memory";
		}
		memset(this->model->voteBase64, 0, 2*len);

		int error = mbedtls_base64_encode(this->model->voteBase64, 2*len - 1, &len, (unsigned char *) zipBuf, len);   //! * base64 encode zip file
		if(error) {
			throw "Base64 encode error";
		}

		mz_free(zipBuf);
		free(this->model->Signature);
		free(this->model->ballot);

		this->vw->setLabel((char*) "Signature complete");
		this->vw->showLoader(false);
	}
	catch(const char *msg){
		this->vw->setLabel((char *) msg);
		this->vw->showLoader(false);
		throw msg;
	}
}
