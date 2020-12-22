#include "controller.h"
#include "view.h"
#include "module.h"

#include "esp_bt.h"


#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"


/**
 * @file BluetoothController.cpp
 * @brief BluetoothController implementation file
 * */

#define GATTS_SERVICE_UUID_TEST_A   0x00FF
#define GATTS_CHAR_UUID_TEST_A      0xFF01
#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     4

#define DEVICE_NAME            "ESP_BOARD"
#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_TAG "GATTS"

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

#define MIN(a,b) (a <= b ? a : b)

static uint8_t char1_str[] = {0x11,0x22,0x33};


#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
        0x02, 0x01, 0x06,
        0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd
};
static uint8_t raw_scan_rsp_data[] = {
        0x0f, 0x09, 0x45, 0x53, 0x50, 0x5f, 0x47, 0x41, 0x54, 0x54, 0x53, 0x5f, 0x44,
        0x45, 0x4d, 0x4f
};
#else

static uint8_t adv_service_uuid128[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00
};

// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

#endif /* CONFIG_SET_RAW_ADV_DATA */
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)



#define PROFILE_A_APP_ID 0

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
    char* data;
};


enum
{
    OVC_IDX_SVC,

    OVC_IDX_VVC_CHAR,
	OVC_IDX_VVC_VAL,
	OVC_IDX_VVC_USR_DESC,
	OVC_IDX_VVC_NTF_CFG,

    OVC_IDX_NB,
};



static const uint16_t ovc_svc = GATTS_SERVICE_UUID_TEST_A;
#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint16_t character_client_description_uuid = ESP_GATT_UUID_CHAR_DESCRIPTION;
static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;

/// Heart Rate Sensor Service - Heart Rate Measurement Characteristic, notify
static const uint16_t vote_verification_svc_uuid = GATTS_CHAR_UUID_TEST_A;
static const uint8_t vote_verification_ccc[2] ={ 0x01, 0x01};

static uint16_t ovc_handle_table[OVC_IDX_NB];


static esp_gatts_attr_db_t attr_db[OVC_IDX_NB] = {
		// Heart Rate Service Declaration
		[OVC_IDX_SVC]                    =
		{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ_ENC_MITM,
		  sizeof(uint16_t), sizeof(ovc_svc), (uint8_t *)&ovc_svc}},

		// Vote Verification Characteristic Declaration
		[OVC_IDX_VVC_CHAR]            =
		{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ_ENC_MITM,
		  CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_notify}},

	  // Vote Verification Characteristic Value
		[OVC_IDX_VVC_VAL]             =
		{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&vote_verification_svc_uuid, ESP_GATT_PERM_READ_ENC_MITM,
		  32,3, char1_str}},

		  // Vote Verification Characteristic User Descriptor
		[OVC_IDX_VVC_USR_DESC]        =
		{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_description_uuid, ESP_GATT_PERM_READ_ENC_MITM,
		  600, 0, NULL}},


		// Vote Verification Characteristic - Client Characteristic Configuration Descriptor
		[OVC_IDX_VVC_NTF_CFG]        =
		{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ_ENC_MITM|ESP_GATT_PERM_WRITE_ENC_MITM,
		  sizeof(uint16_t),sizeof(vote_verification_ccc), (uint8_t *)vote_verification_ccc}},
};


static BLEModule ble;

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static struct gatts_profile_inst PROFILE_A_APP;


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK && param->reg.app_id == PROFILE_A_APP_ID) {
        	PROFILE_A_APP.gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n", param->reg.app_id, param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
		if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
				gatts_if == PROFILE_A_APP.gatts_if) {
			if (PROFILE_A_APP.gatts_cb) {
				PROFILE_A_APP.gatts_cb(event, gatts_if, param);
			}
		}

    } while (0);
}

static uint8_t BT_ON = 0;
static uint8_t data_sent = 0;

uint16_t mtuVal = 23;
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {


    switch (event) {
    case ESP_GATTS_REG_EVT:{
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);

        esp_err_t set_dev_name_ret;
		set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
        if (set_dev_name_ret){
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
        esp_ble_gap_config_local_privacy(true);
        attr_db[OVC_IDX_VVC_USR_DESC].att_desc.length = (uint16_t) strlen( PROFILE_A_APP.data);
        attr_db[OVC_IDX_VVC_USR_DESC].att_desc.value = (uint8_t *) PROFILE_A_APP.data;

		esp_ble_gatts_create_attr_tab(attr_db, gatts_if, OVC_IDX_NB, PROFILE_A_APP_ID);

        break;
    }
    case ESP_GATTS_READ_EVT:{
        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;

		size_t len = MIN(mtuVal -1, strlen(PROFILE_A_APP.data) - param->read.offset);
		rsp.attr_value.len = len ;
		printf("%d %d %d %d %d\n", len, rsp.attr_value.len, rsp.attr_value.offset, param->read.offset, param->read.is_long);
		memcpy(rsp.attr_value.value, PROFILE_A_APP.data + param->read.offset, len);

		esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);

        break;
    }
    case ESP_GATTS_WRITE_EVT:{
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep){
            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
            ESP_LOGI(GATTS_TAG, "%u %u", PROFILE_A_APP.descr_handle,  param->write.handle);
            if (param->write.len == 2){
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                printf("%d %04x \n", *(param->write.value), descr_value);
                if(descr_value == 0x0101){
                	ESP_LOGI(GATTS_TAG, "data received");
                	esp_err_t ret;
                	data_sent = 1;
                	ret = esp_ble_gap_disconnect(param->write.bda);
                	if(ret != ESP_OK){
                		ESP_LOGE(GATTS_TAG, "unable to disconnect the device");
                	}

                }else{
                    ESP_LOGE(GATTS_TAG, "unallowed operations");
                    esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
                }
            }
        }
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        break;
    case ESP_GATTS_MTU_EVT:
    	mtuVal = param->mtu.mtu;
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", mtuVal);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:{
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);

        break;
    }
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT: {
        //start sent the update connection parameters to the peer device.
//        esp_ble_gap_update_conn_params(&conn_params);
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
		if(!data_sent)
			esp_ble_gap_start_advertising(ble.adv);
		else
			BT_ON=1;
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
		ESP_LOGI(GATTS_TAG, "The number handle = %x",param->add_attr_tab.num_handle);
		if (param->create.status == ESP_GATT_OK){
			if(param->add_attr_tab.num_handle == OVC_IDX_NB) {
				memcpy(ovc_handle_table, param->add_attr_tab.handles, sizeof(ovc_handle_table));
				esp_ble_gatts_start_service(ovc_handle_table[OVC_IDX_SVC]);
			}else{
				ESP_LOGE(GATTS_TAG, "Create attribute table abnormally, num_handle (%d) doesn't equal to OVC_IDX_NB(%d)",
					 param->add_attr_tab.num_handle, OVC_IDX_NB);
			}
		}else{
			ESP_LOGE(GATTS_TAG, " Create attribute table failed, error code = %x", param->create.status);
		}
    }
		break;
    case ESP_GATTS_OPEN_EVT:
	case ESP_GATTS_CANCEL_OPEN_EVT:
	case ESP_GATTS_CLOSE_EVT:
	case ESP_GATTS_LISTEN_EVT:
	case ESP_GATTS_CONGEST_EVT:
	case ESP_GATTS_SEND_SERVICE_CHANGE_EVT:
    default:
        break;
    }
}

BluetoothController::BluetoothController(BaseModel *model){
	this->vw = new IndexView();
	this->model = static_cast<BluetoothModel *>(model);
}


void BluetoothController::index(){
	try{
		ble = BLEModule();
		esp_err_t ret = ble.init();  //!  * init bluetooth module, and start advertising
		this->vw->render((void*) "bluetooth on");
		BT_ON = 1;

		PROFILE_A_APP.gatts_cb = gatts_profile_a_event_handler;
		PROFILE_A_APP.gatts_if = ESP_GATT_IF_NONE;       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */

		ret = esp_ble_gatts_register_callback(gatts_event_handler);  //! * register callback function for bletooth events
		if (ret){
			ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
			throw "TAG error";
		}
		size_t len = strlen(this->model->ssid) + strlen(this->model->voteID) + strlen((char *)this->model->rndBase64) + 3;
		PROFILE_A_APP.data = (char *) malloc(len);
		memset(PROFILE_A_APP.data, 0 , len);
		snprintf(PROFILE_A_APP.data, len, "%s\n%s\n%s",this->model->ssid,this->model->rndBase64,this->model->voteID);  //! * prepare data to send
		ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);  //! * register app profile
		if (ret){
			ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
			throw "BLE error";
		}
		esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
		if (local_mtu_ret){
			ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
			throw "BLE error";
		}


		/* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
		esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM;     //bonding with peer device after authentication
		esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;           //set the IO capability to No output No input
		uint8_t key_size = 16;      //the key size should be 7~16 bytes
		uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
		uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
		//set static passkey
		uint32_t passkey = esp_random() % 999999;
		char bt_pin[8];
		snprintf(bt_pin, 7, "%06u", passkey);
		this->vw->render((void *) bt_pin);
		uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
		uint8_t oob_support = ESP_BLE_OOB_DISABLE;
		esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
		esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
		esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
		esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
		esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
		esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
		/* If your BLE device acts as a Slave, the init_key means you hope which types of key of the master should distribute to you,
		and the response key means which key you can distribute to the master;
		If your BLE device acts as a master, the response key means you hope which types of key of the slave should distribute to you,
		and the init key means which key you can distribute to the slave. */
		esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
		esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));


		while(BT_ON && !data_sent){
			vTaskDelay(500 / portTICK_PERIOD_MS);    //! * wait while bluetooth task is running
		}
		ret = esp_ble_gatts_app_unregister(PROFILE_A_APP_ID);
		if(ret != ESP_OK){
			ESP_LOGE(GATTS_TAG, "unable to unregister the app profile");
		}
		ESP_LOGI(GATTS_TAG, "gatt server disconnected.");
		ret = ble.deinit();  //! * turn off bluetooth
		this->vw->render((void*) "bluetooth off");
	}
	catch (const char* msg){
		this->vw->render((void *)msg);
		throw msg;
	}
}
