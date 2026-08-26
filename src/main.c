/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @brief Zigbee application template.
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

//GPIO
#include <zephyr/drivers/gpio.h>

//ADC
#include <zephyr/drivers/adc.h>

//to easily control board-level LEDs and buttons on Nordic development boards
//#include <dk_buttons_and_leds.h>

#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_nrf_platform.h>

//my custom cluser 
#include "zb_alarm_sensor.h"
#include "zb_power.h"

//Contiains device informations
#include "zb_basic_attr.h"

//Used for power_down_unused_ram()
#include <ram_pwrdn.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF); //LOG_LEVEL_INF

#define MANUFACTOR_CODE ZB_ZCL_NON_MANUFACTURER_SPECIFIC

/* Device endpoint, used to receive ZCL commands. */
#define APP_POWER_EP 1
#define APP_ALARM_ACTIVATION_EP 2
#define APP_ALARM_INT_EP 3
#define APP_ALARM_EXT_SUD_EP 4
#define APP_ALARM_EXT_NORD_EP 5

/* Button used to enter the Identify mode. */
//#define IDENTIFY_MODE_BUTTON                DK_BTN4_MSK

/* Button to start Factory Reset */
//#define FACTORY_RESET_BUTTON                IDENTIFY_MODE_BUTTON

/*Alarms GPIO definition*/
static const struct gpio_dt_spec alarm_state = GPIO_DT_SPEC_GET(DT_ALIAS(alarmstate), gpios);
static const struct gpio_dt_spec nord_state = GPIO_DT_SPEC_GET(DT_ALIAS(nordstate), gpios);
static const struct gpio_dt_spec sud_state = GPIO_DT_SPEC_GET(DT_ALIAS(sudstate), gpios);
static const struct gpio_dt_spec internal_state = GPIO_DT_SPEC_GET(DT_ALIAS(internalstate), gpios);

/*Battery gpio*/
static const struct gpio_dt_spec enable_voltage_read = GPIO_DT_SPEC_GET(DT_ALIAS(voltageread), gpios);

//Led
static const struct gpio_dt_spec led_blue =  GPIO_DT_SPEC_GET(DT_ALIAS(ledblue), gpios);
static const struct gpio_dt_spec led_green =  GPIO_DT_SPEC_GET(DT_ALIAS(ledgreen), gpios);
static const struct gpio_dt_spec led_red =  GPIO_DT_SPEC_GET(DT_ALIAS(ledred), gpios);

//ADC init
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
//ADC buffer Define and initialize a sequence to store samples captured by the ADC.
#define ADC_SAMPLES  100

//prototypes for helpers
static int set_active_gpio(const struct gpio_dt_spec* gpio);
static int set_inactive_gpio(const struct gpio_dt_spec* gpio);
static int toggle_gpio(const struct gpio_dt_spec* gpio);

//prototype for gpio callback
void gpio_callback(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins);

//gpio callback data
static struct gpio_callback pin_cb_data_alarm_state;
static struct gpio_callback pin_cb_data_nord_state;
static struct gpio_callback pin_cb_data_sud_state;
static struct gpio_callback pin_cb_data_internal_state;

struct  gpio_callback_struct{
	struct k_work gpio_callback_work;
	uint32_t call_pin;
}; //create thread on callback to send zigbee update
static struct gpio_callback_struct gpio_callback_work_with_params_alarm;
static struct gpio_callback_struct gpio_callback_work_with_params_sud;
static struct gpio_callback_struct gpio_callback_work_with_params_nord;
static struct gpio_callback_struct gpio_callback_work_with_params_int;
/*static struct k_work gpio_callback_work_alarm;
static struct k_work gpio_callback_work_sud;
static struct k_work gpio_callback_work_nord;
static struct k_work gpio_callback_work_int;
*/

//timer callback
static struct k_timer timer_interrupt;
static struct k_work timer_callback_work;



/////////////////////////////START ZIGBEE ENDPOINTS/////////////////////////////////////////
//BASIC and IDENTIFIES attributes. Add the same into every EP.
struct zb_device_ctx {
	zb_zcl_basic_attrs_ext_t  basic_attr;
	zb_zcl_identify_attrs_t identify_attr;
};

/* Zigbee device application context storage. */
struct zb_device_ctx device_basic;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT
(
	basic_attr_list,
	&device_basic.basic_attr.zcl_version,
	&device_basic.basic_attr.app_version,
	&device_basic.basic_attr.stack_version,
	&device_basic.basic_attr.hw_version,
	device_basic.basic_attr.mf_name,
	device_basic.basic_attr.model_id,
	device_basic.basic_attr.date_code,
	&device_basic.basic_attr.power_source,
	device_basic.basic_attr.location_id,
	&device_basic.basic_attr.ph_env,
	device_basic.basic_attr.sw_ver
);

/* Declare attribute list for Identify cluster (server). */
ZB_ZCL_DECLARE_IDENTIFY_SERVER_ATTRIB_LIST(
	identify_attr_list,
	&device_basic.identify_attr.identify_time);

//////////////////////END BASIC and IDENTIFIES//////////////////////

/////Binary input cluster and EP////////
struct my_binary_input_attr_list_s {
	zb_bool_t   out_of_service;
	zb_bool_t   present_value;
	zb_uint8_t  status_flag;
	char  description[50];
};

//Alarm ext EP
struct my_binary_input_attr_list_s alarm_activation;
struct my_binary_input_attr_list_s alarm_int;
struct my_binary_input_attr_list_s alarm_ext_sud;
struct my_binary_input_attr_list_s alarm_ext_nord;

///////////////////////////alarm_activation//////////////////////////
//Binary input cluster attribute 
ZB_ZCL_DECLARE_BINARY_INPUT_ATTRIB_LIST_EXT	(	 	
	alarm_activation_attr_list,
 	&alarm_activation.out_of_service,
 	&alarm_activation.present_value,
 	&alarm_activation.status_flag,
	alarm_activation.description
);

//Pack all clusters into EP
ZB_DECLARE_ALARM_SENSOR_CLUSTER_LIST(					  
	alarm_activation_cluster_list,
	basic_attr_list,				  
	identify_attr_list,
	alarm_activation_attr_list
);

//Declare EP
ZB_DECLARE_ALARM_SENSOR_EP(
	alarm_activation_ep,
	APP_ALARM_ACTIVATION_EP,
	alarm_activation_cluster_list
);
////////////////End alarm_activation/////////////////////

///////////////// alarm_int////////////////////////////
//Binary input cluster attribute
ZB_ZCL_DECLARE_BINARY_INPUT_ATTRIB_LIST_EXT	(	 	
	alarm_int_attr_list,
 	&alarm_int.out_of_service,
 	&alarm_int.present_value,
 	&alarm_int.status_flag,
	alarm_int.description
);

//Pack all clusters into EP
ZB_DECLARE_ALARM_SENSOR_CLUSTER_LIST(					  
	alarm_int_cluster_list,
	basic_attr_list,				  
	identify_attr_list,
	alarm_int_attr_list
);

//Declare EP
ZB_DECLARE_ALARM_SENSOR_EP(
	alarm_int_ep,
	APP_ALARM_INT_EP,
	alarm_int_cluster_list
);
///////////////////////End  alarm_int////////////////////////////

//////////////////////alarm_ext_sud///////////////////////////
//Binary input cluster attribute
ZB_ZCL_DECLARE_BINARY_INPUT_ATTRIB_LIST_EXT	(	 	
	alarm_ext_sud_attr_list,
 	&alarm_ext_sud.out_of_service,
 	&alarm_ext_sud.present_value,
 	&alarm_ext_sud.status_flag,
	alarm_ext_sud.description
);

//Pack all clusters into EP
ZB_DECLARE_ALARM_SENSOR_CLUSTER_LIST(					  
	alarm_ext_sud_cluster_list,
	basic_attr_list,				  
	identify_attr_list,
	alarm_ext_sud_attr_list
);

//Declare EP
ZB_DECLARE_ALARM_SENSOR_EP(
	alarm_ext_sud_ep,
	APP_ALARM_EXT_SUD_EP,
	alarm_ext_sud_cluster_list
);
/////////////////////End alarm_ext_sud/////////////////////////////


//////////////////////alarm_ext_nord///////////////////////////
//Binary input cluster attribute
ZB_ZCL_DECLARE_BINARY_INPUT_ATTRIB_LIST_EXT	(	 	
	alarm_ext_nord_attr_list,
 	&alarm_ext_nord.out_of_service,
 	&alarm_ext_nord.present_value,
 	&alarm_ext_nord.status_flag,
	alarm_ext_nord.description
);

//Pack all clusters into EP
ZB_DECLARE_ALARM_SENSOR_CLUSTER_LIST(					  
	alarm_ext_nord_cluster_list,
	basic_attr_list,				  
	identify_attr_list,
	alarm_ext_nord_attr_list
);

//Declare EP
ZB_DECLARE_ALARM_SENSOR_EP(
	alarm_ext_nord_ep,
	APP_ALARM_EXT_NORD_EP,
	alarm_ext_nord_cluster_list
);
/////////////////////End alarm_ext_nord/////////////////////////////

//-------Battery----//
/*
struct my_analog_input_attr_list_s {
    zb_uint8_t description[50];
    zb_single_t max_present_value;
    zb_single_t min_present_value;
    zb_bool_t   out_of_service;
    zb_single_t present_value;     / The actual sensor value
    zb_uint8_t  reliability;
    zb_single_t resolution;
    zb_uint8_t  status_flags;
    zb_uint16_t engineering_units; // Used to explicitly declare unit type  SET THE MEASUREMENT UNIT (0x0005 = Volts, 0x00E1 = Millivolts, 0x001D: Percent (%))
    zb_uint32_t app_type;
};

struct my_analog_input_attr_list_s battery_voltage_attr;

//Analog input attributes
ZB_ZCL_DECLARE_ANALOG_INPUT_ATTRIB_LIST(
		voltage_sensor_attr_list,
		battery_voltage_attr.description,
		&battery_voltage_attr.max_present_value,
		&battery_voltage_attr.min_present_value,
		&battery_voltage_attr.out_of_service,
		&battery_voltage_attr.present_value,
		&battery_voltage_attr.reliability,
		&battery_voltage_attr.resolution,
		&battery_voltage_attr.status_flags,
		&battery_voltage_attr.engineering_units,
		&battery_voltage_attr.app_type
);

//Pack all cluster
ZB_DECLARE_BATTERY_SENSOR_CLUSTER_LIST(
	battery_voltage_cluster_list,
	basic_attr_list,				  
	identify_attr_list,
	voltage_sensor_attr_list
);

//Declare EP
ZB_DECLARE_BATTERY_SENSOR_EP(
	voltageep,
	APP_BATTERY_VOLTAGE_EP,
	battery_voltage_cluster_list
);*/
////End Voltage

//POWER EP (Battery voltage and percentage)///
struct app_power_config_ctx_t {
    zb_uint8_t voltage; // Attribute 3.3.2.2.3.1
    zb_uint8_t size; // Attribute 3.3.2.2.4.2
    zb_uint8_t quantity; // Attribute 3.3.2.2.4.4
    zb_uint8_t rated_voltage; // Attribute 3.3.2.2.4.5
    zb_uint8_t alarm_mask; // Attribute 3.3.2.2.4.6
    zb_uint8_t voltage_min_threshold; // Attribute 3.3.2.2.4.7
    zb_uint8_t percent_remaining; // Attribute 3.3.2.2.3.1
    zb_uint8_t voltage_threshold_1; // Attribute 3.3.2.2.4.8
    zb_uint8_t voltage_threshold_2; // Attribute 3.3.2.2.4.8
    zb_uint8_t voltage_threshold_3; // Attribute 3.3.2.2.4.8
    zb_uint8_t percent_min_threshold; // Attribute 3.3.2.2.4.9
    zb_uint8_t percent_threshold_1; // Attribute 3.3.2.2.4.10
    zb_uint8_t percent_threshold_2; // Attribute 3.3.2.2.4.10
    zb_uint8_t percent_threshold_3; // Attribute 3.3.2.2.4.10
    zb_uint32_t alarm_state; // Attribute 3.3.2.2.4.11
};

struct app_power_config_ctx_t power_attr;

//Set power cluster attribute list
//In power cluster you can define this or leave blank. Mandatory for compilation
#define bat_num
ZB_ZCL_DECLARE_POWER_CONFIG_BATTERY_ATTRIB_LIST_EXT(
	power_attr_list,
	&power_attr.voltage,
	&power_attr.size,
	&power_attr.quantity,
	&power_attr.rated_voltage,
	&power_attr.alarm_mask,
	&power_attr.voltage_min_threshold,
	&power_attr.percent_remaining,
	&power_attr.voltage_threshold_1,
	&power_attr.voltage_threshold_2,
	&power_attr.voltage_threshold_3,
	&power_attr.percent_min_threshold,
	&power_attr.percent_threshold_1,
	&power_attr.percent_threshold_2,
	&power_attr.percent_threshold_3,
	&power_attr.alarm_state
);

//Pack all clusters
ZB_DECLARE_POWER_CLUSTER_LIST(
	power_cluster_list,
	basic_attr_list,				  
	identify_attr_list,
	power_attr_list
);

//Define EP
ZB_DECLARE_POWER_EP(
	powerep,
	APP_POWER_EP,
	power_cluster_list
);

//End power

//DEVICE CTX. All EP
zb_af_endpoint_desc_t *my_ep_array[] = {
    &powerep,
    &alarm_activation_ep,
    &alarm_int_ep,
    &alarm_ext_sud_ep,
    &alarm_ext_nord_ep,
    // Add as many as needed
};

/*ZBOSS_DECLARE_DEVICE_CTX_5_EP(
	app_template_ctx,
	powerep,
	alarm_activation_ep,
	alarm_int_ep,
	alarm_ext_sud_ep,
	alarm_ext_nord_ep
);*/

//Use this for more then 4 ep
ZBOSS_DECLARE_DEVICE_CTX(
    app_template_ctx, 
    my_ep_array,                    // Pointer to your array of endpoints
    ZB_ARRAY_SIZE(my_ep_array)     // Total endpoint count (>4)
);
/////////////////////////////END ZIGBEE ENDPOINTS/////////////////////////////////////////

/*@brief Function for initializing all clusters attributes. */
static void app_basic_clusters_attr_init(void)
{

	//-----------------Basic attribute init---------------------
	device_basic.basic_attr.zcl_version   = ZB_ZCL_VERSION;
	device_basic.basic_attr.power_source  = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
    device_basic.basic_attr.power_source  = BASIC_POWER_SOURCE;
    device_basic.basic_attr.stack_version = BASIC_STACK_VERSION;
    device_basic.basic_attr.hw_version    = BASIC_HW_VERSION;

    // Use ZB_ZCL_SET_STRING_VAL to set strings, because the first byte should
    // contain string length without trailing zero.
    //
    // For example "test" string wil be encoded as:
    //   [(0x4), 't', 'e', 's', 't']

    ZB_ZCL_SET_STRING_VAL(device_basic.basic_attr.mf_name,
                          BASIC_MANUF_NAME,
                          ZB_ZCL_STRING_CONST_SIZE(BASIC_MANUF_NAME));

    ZB_ZCL_SET_STRING_VAL(device_basic.basic_attr.model_id,
                          BASIC_MODEL_ID,
                          ZB_ZCL_STRING_CONST_SIZE(BASIC_MODEL_ID));

    ZB_ZCL_SET_STRING_VAL(device_basic.basic_attr.date_code,
                          BASIC_DATE_CODE,
                          ZB_ZCL_STRING_CONST_SIZE(BASIC_DATE_CODE));


    ZB_ZCL_SET_STRING_VAL(device_basic.basic_attr.location_id,
                          BASIC_LOCATION_DESC,
                          ZB_ZCL_STRING_CONST_SIZE(BASIC_LOCATION_DESC));


    device_basic.basic_attr.ph_env = BASIC_PH_ENV;

	// Identify cluster attributes data.
	device_basic.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
	//------------------End Basic--------------------------------
}

void app_binary_input_clusters_attr_init(struct my_binary_input_attr_list_s* binary_input_attr_list, char descriptionArray[]){

	//Binary input
	binary_input_attr_list->out_of_service = ZB_ZCL_BINARY_INPUT_OUT_OF_SERVICE_DEFAULT_VALUE;
	binary_input_attr_list->present_value = ZB_TRUE;
	binary_input_attr_list->status_flag = ZB_ZCL_BINARY_INPUT_STATUS_FLAG_DEFAULT_VALUE;
	ZB_ZCL_SET_STRING_VAL(
		binary_input_attr_list->description, 
		descriptionArray,
		strlen(descriptionArray)
	);
	LOG_INF("app_binary_input_clusters_attr_init descr:%s", descriptionArray);
}
	//Analog input
	/*The values 0x0000 to 0x00FE are used to represent the
	units specified in Clause 21 of the BACnet standard. The value 0x00FF
	represents 'other' unit, and the values 0x0100 to 0xFFFF are for proprietary
	use.*/	
	/*
	battery_voltage_attr.out_of_service = ZB_ZCL_ANALOG_INPUT_OUT_OF_SERVICE_DEFAULT_VALUE;
	battery_voltage_attr.present_value = 0;
	battery_voltage_attr.min_present_value = 0;
	battery_voltage_attr.max_present_value = 10000;
	battery_voltage_attr.engineering_units =  0x00E1; //mV
	battery_voltage_attr.status_flags = ZB_ZCL_ANALOG_INPUT_STATUS_FLAG_NORMAL;
	battery_voltage_attr.reliability = ZB_ZCL_ANALOG_INPUT_RELIABILITY_DEFAULT_VALUE;
	battery_voltage_attr.app_type = ZB_ZCL_AI_APP_TYPE_OTHER; // Configured for Voltage
	battery_voltage_attr.resolution = 1;
	ZB_ZCL_SET_STRING_VAL(
		battery_voltage_attr.description, 
		"analog_val",
		ZB_ZCL_STRING_CONST_SIZE("analog_val")
	);*/
static void app_power_clusters_attr_init(void){

	//Power attr
	power_attr.voltage               = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_INVALID;
	power_attr.size                  = ZB_ZCL_POWER_CONFIG_BATTERY_SIZE_OTHER;
	power_attr.quantity              = 1; //1 battery
	power_attr.rated_voltage         = 42; //4.2V
	power_attr.alarm_mask            = ZB_ZCL_POWER_CONFIG_MAINS_ALARM_MASK_DEFAULT_VALUE;
	power_attr.voltage_min_threshold = ZB_ZCL_POWER_CONFIG_MAINS_VOLTAGE_MIN_THRESHOLD_DEFAULT_VALUE;
	power_attr.percent_remaining     = ZB_ZCL_POWER_CONFIG_BATTERY_REMAINING_UNKNOWN;
	power_attr.voltage_threshold_1   = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD1_DEFAULT_VALUE;
	power_attr.voltage_threshold_2   = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD2_DEFAULT_VALUE;
	power_attr.voltage_threshold_3   = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD3_DEFAULT_VALUE;
	power_attr.percent_min_threshold = ZB_ZCL_POWER_CONFIG_BATTERY_PERCENTAGE_MIN_THRESHOLD_DEFAULT_VALUE;
	power_attr.percent_threshold_1   = ZB_ZCL_POWER_CONFIG_BATTERY_PERCENTAGE_THRESHOLD1_DEFAULT_VALUE;
	power_attr.percent_threshold_2   = ZB_ZCL_POWER_CONFIG_BATTERY_PERCENTAGE_THRESHOLD2_DEFAULT_VALUE;
	power_attr.percent_threshold_3   = ZB_ZCL_POWER_CONFIG_BATTERY_PERCENTAGE_THRESHOLD3_DEFAULT_VALUE;
	power_attr.alarm_state           = ZB_ZCL_POWER_CONFIG_BATTERY_ALARM_STATE_DEFAULT_VALUE;
}


/**@brief Function to toggle the identify LED
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void toggle_identify_led(zb_bufid_t bufid)
{
	//static int blink_status;
	LOG_INF("toggle_identify_led: %u", bufid);
	toggle_gpio(&led_blue);
	ZB_SCHEDULE_APP_ALARM(toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/**@brief Function to handle identify notification events on the first endpoint.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void identify_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;
	LOG_INF("identify_cb bufid: %u", bufid);

	if (bufid) {
		/* Schedule a self-scheduling function that will toggle the LED. */
		ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
		LOG_INF("Schedule a self-scheduling ep_id: %u", bufid);
	} else {
		/* Cancel the toggling function alarm and turn off LED. */
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);

		/* Update network status/idenitfication LED. */
		if (ZB_JOINED()) {
			LOG_INF("Joined ep_id: %u", bufid);
			set_inactive_gpio(&led_green);
			set_inactive_gpio(&led_red);
			set_inactive_gpio(&led_blue);			
		} else {
			LOG_INF("Not joined ep_id: %u", bufid);
			set_inactive_gpio(&led_green);
			set_active_gpio(&led_red);
			set_inactive_gpio(&led_blue);			

		}
	}
}

/**@brief Starts identifying the device.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
/*static void start_identifying(zb_bufid_t bufid)
{
	ZVUNUSED(bufid);

	if (ZB_JOINED()) {
		// Check if endpoint is in identifying mode,cif not put desired endpoint in identifying mode.
		 
		LOG_INF("Check if buf: %u is in identifying mode", bufid);

		if (device_basic.identify_attr.identify_time == ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE) {

			zb_ret_t zb_err_code;

			//zb_ret_t zb_err_code = zb_bdb_finding_binding_target(APP_BATTERY_VOLTAGE_EP);
			zb_err_code = zb_bdb_finding_binding_target(APP_POWER_EP);
			//zb_err_code = zb_bdb_finding_binding_target(APP_ALARM_EXT_EP);

			if (zb_err_code == RET_OK) {
				LOG_INF("Enter identify mode");
			} else if (zb_err_code == RET_INVALID_STATE) {
				LOG_WRN("RET_INVALID_STATE - Cannot enter identify mode");
			} else {
				LOG_WRN("start_identifying error");
				ZB_ERROR_CHECK(zb_err_code);
			}
		} else {
			LOG_INF("Cancel identify mode");
			zb_bdb_finding_binding_target_cancel();
		}
	} else {
		LOG_WRN("Device not in a network - cannot enter identify mode");
	}
}*/

/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons
 *                            that have changed their state.
 */
/*static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (IDENTIFY_MODE_BUTTON & has_changed) {
		if (IDENTIFY_MODE_BUTTON & button_state) {
			// Button changed its state to pressed
		} else {
			// Ceck if long press occured to do factory reset
			if (was_factory_reset_done()) {
				// The long press was for Factory Reset
				LOG_DBG("After Factory Reset - ignore button release");
			} else   {
				// Button released before Factory Reset

				// Start identification mode
				ZB_SCHEDULE_APP_CALLBACK(start_identifying, 0);
			}
		}
	}

	check_factory_reset_button(button_state, has_changed);
}*/

/**@brief Function for initializing LEDs and Buttons. */
/*static void configure_reset_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}*/

/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer
 *                      used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	/* Update network status LED. */
	if(ZB_JOINED()){
		//LOG_INF("zboss_signal_handler JOINED");
		set_inactive_gpio(&led_red);
	}else{
		//LOG_INF("zboss_signal_handler NOT");	
		set_active_gpio(&led_red);
	}
	set_inactive_gpio(&led_green);
	set_inactive_gpio(&led_blue);

	/* No application-specific behavior is required.
	 * Call default signal handler.
	 */
	ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));

	/* All callbacks should either reuse or free passed buffers.
	 * If bufid == 0, the buffer is invalid (not passed).
	 */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

void update_binary_input_value(zb_uint8_t endpoint_id, bool new_state)
{
    // The data type for PresentValue in a Binary Input cluster is a boolean (zb_bool_t)
    zb_uint8_t value_to_set = ZB_FALSE;
	if(new_state){
		value_to_set = ZB_TRUE;
	}

    // Use the ZBOSS ZCL common macro to update the attribute table
    zb_zcl_status_t response = zb_zcl_set_attr_val(
        endpoint_id,                                   // Target Endpoint ID
        ZB_ZCL_CLUSTER_ID_BINARY_INPUT,             // Cluster ID (Binary Input)
        ZB_ZCL_CLUSTER_SERVER_ROLE,                 // Cluster Role (Server)
        ZB_ZCL_ATTR_BINARY_INPUT_PRESENT_VALUE_ID,  // Attribute ID (PresentValue)
         (zb_uint8_t *)&value_to_set,                				// Pointer to the new value
        ZB_FALSE                                    // check_access flag (ZB_FALSE bypasses read-only constraints locally)
    );
	if(response == ZB_ZCL_STATUS_READ_ONLY ){
		LOG_INF("update_binary_input_value ZB_ZCL_STATUS_READ_ONLY");
	}else if(response == ZB_ZCL_STATUS_SUCCESS ){
		LOG_INF("update_binary_input_value ZB_ZCL_STATUS_SUCCESS");
	}else if(response == ZB_ZCL_STATUS_INVALID_VALUE ){
		LOG_INF("update_binary_input_value ZB_ZCL_STATUS_INVALID_VALUE");
	};
}

void update_battery_voltage(float new_voltage, float max_value) {

    // Safely update the underlying structure value
	float perc = (new_voltage/max_value)*100.0f;
	zb_uint8_t perc_val = (zb_uint8_t)(perc * 2.0f);
	zb_uint8_t volt_to_set = (zb_uint8_t)(new_voltage * 10.0f);
	
	LOG_INF("update_battery_voltage v: %f -> %u", (double)new_voltage, volt_to_set);
	LOG_INF("update_battery_voltage perc: %f -> %u", (double)perc, perc_val);

    //Trigger an attribute stack refresh (notifies listening links and handles reporting bindings)
    /*zb_zcl_status_t voltage_status = zb_zcl_set_attr_val_manuf(
        APP_BATTERY_VOLTAGE_EP,
        ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID,
		MANUFACTOR_CODE,
        (zb_uint8_t *)&volt_to_set,
        ZB_FALSE
    );	
	if(voltage_status == ZB_ZCL_STATUS_READ_ONLY ){
		LOG_INF("voltage_status ZB_ZCL_STATUS_READ_ONLY");
	}else if(voltage_status == ZB_ZCL_STATUS_SUCCESS ){
		LOG_INF("voltage_status ZB_ZCL_STATUS_SUCCESS");
	}else if(voltage_status == ZB_ZCL_STATUS_INVALID_VALUE ){
		LOG_INF("voltage_status ZB_ZCL_STATUS_INVALID_VALUE");
	};*/

	//Perc
	zb_zcl_status_t battery_perc_rep_status = zb_zcl_set_attr_val(
        APP_POWER_EP,
        ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
        (zb_uint8_t *)&perc_val, //The pointer MUST be 8bit
        ZB_FALSE
    );
	if(battery_perc_rep_status == ZB_ZCL_STATUS_READ_ONLY ){
		LOG_INF("battery_perc_rep_status ZB_ZCL_STATUS_READ_ONLY");
	}else if(battery_perc_rep_status == ZB_ZCL_STATUS_SUCCESS ){
		LOG_INF("battery_perc_rep_status ZB_ZCL_STATUS_SUCCESS");
	}else if(battery_perc_rep_status == ZB_ZCL_STATUS_INVALID_VALUE ){
		LOG_INF("battery_perc_rep_status ZB_ZCL_STATUS_INVALID_VALUE");
	};

	//Voltage
	zb_zcl_status_t battery_volt_rep_status = zb_zcl_set_attr_val(
        APP_POWER_EP,
        ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
        (zb_uint8_t *)&volt_to_set, //The pointer MUST be 8bit
        ZB_FALSE
    );

	if(battery_volt_rep_status == ZB_ZCL_STATUS_READ_ONLY ){
		LOG_INF("battery_volt_rep_status ZB_ZCL_STATUS_READ_ONLY");
	}else if(battery_volt_rep_status == ZB_ZCL_STATUS_SUCCESS ){
		LOG_INF("battery_volt_rep_status ZB_ZCL_STATUS_SUCCESS");
	}else if(battery_volt_rep_status == ZB_ZCL_STATUS_INVALID_VALUE ){
		LOG_INF("battery_volt_rep_status ZB_ZCL_STATUS_INVALID_VALUE");
	}

	//mark battery voltage reportable. This crash Zigbee2Mqtt
	/*zb_zcl_mark_attr_for_reporting(
		APP_POWER_EP,
		ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID
	);*/	

}

/*All binary_input go here*/
int configure_gpio_in(const struct gpio_dt_spec* gpio){

	if (!gpio_is_ready_dt(gpio)) {
		return 0;
	}
	gpio_pin_configure_dt(gpio, GPIO_INPUT);

	return 1;
}

/*All output gpios go here*/
int configure_gpio_out(const struct gpio_dt_spec* gpio){
	
	if (!gpio_is_ready_dt(gpio)) {
		return 0;
	}
	gpio_pin_configure_dt(gpio, GPIO_OUTPUT);
	
	return 1;	
}

int set_active_gpio(const struct gpio_dt_spec* gpio){
	gpio_pin_set_dt(gpio, 1);
	return 1;
}

int set_inactive_gpio(const struct gpio_dt_spec* gpio){
	gpio_pin_set_dt(gpio, 0);
	return 1;
}

int toggle_gpio(const struct gpio_dt_spec* gpio){
	 gpio_pin_toggle_dt(gpio);
	 return 1;
}

/*Get effective battery value. ADC measure then divider voltage*/
float getBatteryValue(float adc_value){
  float r1 = 27*10^3;
  float r2 = 100*10^3;
  float alpha = r2/(r1+r2); 
  return ( (float)adc_value / alpha );
}

int battery_report(){

	//Enable voltage divider
	set_active_gpio(&enable_voltage_read);

	//Stabilize
	k_msleep(500);

	struct adc_sequence_options options = {
		// Number of extra samples *beyond* the first one
		.extra_samplings = ADC_SAMPLES - 1,
		//Interval between consecutive samplings (in microseconds), 0 means sample as fast as possible, without involving any timer.
		.interval_us = 0,
		.callback = NULL,
		.user_data = NULL,
	};

	//Buffer in which store sampples
	int16_t adc_samples_buffer[ADC_SAMPLES];
	struct adc_sequence sequence = {
		.options = &options,
		.buffer = adc_samples_buffer, // &adc_samples_buffer,
		// buffer size in bytes, not number of samples
		.buffer_size = sizeof(adc_samples_buffer),
		//Optional
		//.calibrate = true,
	};

	int err = adc_sequence_init_dt(&adc_channel, &sequence);
	if (err < 0) {
		LOG_ERR("Could not initalize sequence");
		return 0;
	}

	//Read values
	err = adc_read(adc_channel.dev, &sequence);
	if (err < 0) {
		LOG_ERR("Could not read (%d)", err);
		return 0;
	} 

	//Disable voltage divider
	set_inactive_gpio(&enable_voltage_read);
	
	//Show values
	float meanVolt = 0;
	for (int i = 0; i < ADC_SAMPLES; i++) {
		int32_t val_mv = adc_samples_buffer[i];
        
        int err = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
        
        if (err < 0) {
            LOG_INF("Sample %d: Raw = %d, mV = Not available\n", i, adc_samples_buffer[i]);
        } else {
            //LOG_INF("Sample %d: Raw = %d, mV = %d mV\n", i, adc_samples_buffer[i], val_mv);
			meanVolt += (int)val_mv;
        }

    }
	meanVolt = meanVolt / ADC_SAMPLES;
    LOG_INF("Measured mV = %d mV", (int)meanVolt);
	float battery_voltage = getBatteryValue(meanVolt);
	LOG_INF("Battery mV = %d mV", (int)battery_voltage);
	update_battery_voltage((float)(battery_voltage/1000.0f), 4.2f); //in volt

	return 1;
}

void read_gpio_state(zb_uint8_t ep_id, const struct gpio_dt_spec* gpio){

	//Enable/dis state. this get logical level of an input pin from
	int enable_val = gpio_pin_get_dt(gpio);
	
	//Update zigbee state
	if(enable_val == 1){
		LOG_INF("alarm state true");
		update_binary_input_value(ep_id, true);
	}

	if(enable_val == 0){
		LOG_INF("alarm state false");
		update_binary_input_value(ep_id, false);
	}
}

int config_adc_channel(){
	if (!adc_is_ready_dt(&adc_channel)) {
		LOG_ERR("ADC controller devivce %s not ready", adc_channel.dev->name);
		return 0;
	}

	int err = adc_channel_setup_dt(&adc_channel);
	if (err < 0) {
		LOG_ERR("Could not setup channel #%d (%d)", 0, err);
		return 0;
	}
	return 1;
}

// Linker stub: If the pre-compiled library lacks the symbol, 
// this satisfies the linker while keeping your cluster list active.
void zb_zcl_analog_input_init_server(void) {
    // Left empty intentionally to satisfy the linker
}
void zb_zcl_analog_input_init_client(void) {
    // Left empty intentionally to satisfy the linker
}

void configureReportingAlarm(int ep_id){

	zb_ret_t ret_code;

	zb_zcl_reporting_info_t rep_config_alarm;
	memset(&rep_config_alarm, 0, sizeof(rep_config_alarm));

	rep_config_alarm.direction             = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
	rep_config_alarm.ep                    = ep_id;
	rep_config_alarm.cluster_id            = ZB_ZCL_CLUSTER_ID_BINARY_INPUT;
	rep_config_alarm.cluster_role          = ZB_ZCL_CLUSTER_SERVER_ROLE;
	rep_config_alarm.manuf_code 		    = MANUFACTOR_CODE;
	rep_config_alarm.attr_id               = ZB_ZCL_ATTR_BINARY_INPUT_PRESENT_VALUE_ID;
	rep_config_alarm.u.send_info.min_interval = ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT;   // 10 seconds min
	rep_config_alarm.u.send_info.max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;  //If Maximum reporting interval is set to value 0xFFFF,reporting is not needed for current attribute.
	rep_config_alarm.u.send_info.delta.u8 = 0x00; // Value change threshold

	// Inject configuration first
	ret_code = zb_zcl_put_reporting_info(&rep_config_alarm, ZB_TRUE);
	if(ret_code != RET_OK){
		LOG_ERR("error configureReportingAlarm ep: %d, error: %d", ep_id, ret_code);
	}else{
		LOG_INF("configureReportingAlarm ep: %d:", ep_id);
	};
}

/*void configureReportingAnalogInput(void){

	zb_ret_t ret_code;

	zb_zcl_reporting_info_t config_analog_input;
	memset(&config_analog_input, 0, sizeof(config_analog_input));

	config_analog_input.direction             = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
	config_analog_input.ep                    = APP_BATTERY_VOLTAGE_EP;
	config_analog_input.cluster_id            = ZB_ZCL_CLUSTER_ID_ANALOG_INPUT;
	config_analog_input.cluster_role          = ZB_ZCL_CLUSTER_SERVER_ROLE;
	config_analog_input.attr_id               = ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID;
	config_analog_input.manuf_code 		      = MANUFACTOR_CODE;
	config_analog_input.u.send_info.min_interval = ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT; 
	config_analog_input.u.send_info.max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
	config_analog_input.u.send_info.delta.u8 = 0x00; // Value change threshold
	//config_analog_input.u.send_info.reported_value.u8 = 0;
	
	// Inject configuration first
	ret_code = zb_zcl_put_reporting_info(&config_analog_input, ZB_TRUE);
	if(ret_code != RET_OK){
		LOG_ERR("error configureReportingAnalogInput voltage  %d", ret_code);
	}else{
		LOG_INF("configureReportingAnalogInput voltage ok");
	};
}*/

void configureReportingPower(int ep_id){

	zb_ret_t ret_code;

	//Battery voltage reporting
	zb_zcl_reporting_info_t config_power;
	memset(&config_power, 0, sizeof(config_power));
	config_power.direction             = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
	config_power.ep                    = ep_id;
	config_power.cluster_id            = ZB_ZCL_CLUSTER_ID_POWER_CONFIG;
	config_power.cluster_role          = ZB_ZCL_CLUSTER_SERVER_ROLE;
	config_power.attr_id               = ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID;
	config_power.manuf_code 		   = MANUFACTOR_CODE;
	//config_power.dst.short_addr = 0x0000;
	//config_power.dst.endpoint = 1;
	config_power.dst.profile_id = ZB_AF_HA_PROFILE_ID;
	config_power.u.send_info.min_interval =  ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT;
	config_power.u.send_info.max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
	config_power.u.send_info.delta.u8 = 0x00;
	config_power.u.send_info.reported_value.u8 = 0;
	config_power.u.send_info.def_min_interval = ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT;
	config_power.u.send_info.def_max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
	ret_code = zb_zcl_put_reporting_info(&config_power, ZB_TRUE);
	if(ret_code != RET_OK){
		LOG_ERR("error configureReportingPower voltage  %d", ret_code);
	}else{
		LOG_INF("configureReportingPower voltage ok");
	};

	//Percentage reporting
	memset(&config_power, 0, sizeof(config_power));
	config_power.direction             = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
	config_power.ep                    = ep_id;
	config_power.cluster_id            = ZB_ZCL_CLUSTER_ID_POWER_CONFIG;
	config_power.cluster_role          = ZB_ZCL_CLUSTER_SERVER_ROLE;
	config_power.attr_id               = ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID;
	config_power.manuf_code 		   = MANUFACTOR_CODE;
	config_power.dst.profile_id = ZB_AF_HA_PROFILE_ID;
	config_power.u.send_info.min_interval = ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT;
	config_power.u.send_info.max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
	config_power.u.send_info.delta.u8 = 0x00;
	config_power.u.send_info.reported_value.u8 = 0;
	config_power.u.send_info.def_min_interval = ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT;
	config_power.u.send_info.def_max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
	ret_code = zb_zcl_put_reporting_info(&config_power, ZB_TRUE);
	if(ret_code != RET_OK){
		LOG_ERR("error configureReportingPower percentage  %d", ret_code);
	}else{
		LOG_INF("configureReportingPower percentage ok");
	};
}

void startReportingAlarm(int ep_id){

	zb_ret_t result;
	
	//Voltage
	result = zb_zcl_start_attr_reporting_manuf(
        ep_id,
        ZB_ZCL_CLUSTER_ID_BINARY_INPUT,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_BINARY_INPUT_PRESENT_VALUE_ID,
		MANUFACTOR_CODE
    );
	if (result != RET_OK) {
    	// RET_NOT_FOUND means reporting context or attribute entry is missing
    	// RET_INVALID_PARAMETER means endpoint/cluster role mismatches
    	LOG_ERR("Start reporting binary input: error %d", result); 
	}else{
		LOG_INF("Start reporting binary input ep: %d", ep_id); 
	}
}

void startReportingPower(int ep_id){

	zb_ret_t result;

	//percentage
	result = zb_zcl_start_attr_reporting_manuf(
        APP_POWER_EP,
        ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
		MANUFACTOR_CODE
    );

	if (result != RET_OK) {
    	// RET_NOT_FOUND means reporting context or attribute entry is missing
    	// RET_INVALID_PARAMETER means endpoint/cluster role mismatches
    	LOG_ERR("Start reporting config_power_perc: error %d", result); 
	}else{
		LOG_INF("Start reporting config_power_perc ok"); 
	}

	//Voltage
	result = zb_zcl_start_attr_reporting_manuf(
        APP_POWER_EP,
        ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
		MANUFACTOR_CODE
    );
	if (result != RET_OK) {
    	// RET_NOT_FOUND means reporting context or attribute entry is missing
    	// RET_INVALID_PARAMETER means endpoint/cluster role mismatches
    	LOG_ERR("Start reporting config_power_voltage: error %d", result); 
	}else{
		LOG_INF("Start reporting config_power_voltage ok"); 
	}
}

/*void startReportingAnalogInput(void){

	zb_ret_t result;

	//Start anlog input attribute reporting
	result = zb_zcl_start_attr_reporting_manuf(
        APP_BATTERY_VOLTAGE_EP,
        ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID,
		MANUFACTOR_CODE
    );

	if(result != RET_OK) {
    	// RET_NOT_FOUND means reporting context or attribute entry is missing
    	// RET_INVALID_PARAMETER means endpoint/cluster role mismatches
    	LOG_ERR("Start reporting config_analog: error %d", result); 
	}else{
		LOG_INF("Reporting config_analog ok"); 
	}
}*/

void gpio_callback_work_handler(struct k_work *work){
	struct gpio_callback_struct* data = CONTAINER_OF(work, struct gpio_callback_struct, gpio_callback_work);
	//LOG_INF("gpio_callback from enablealarm_state_alarm pin: %d", data->call_pin);
	if(data->call_pin == 32768){
		read_gpio_state(APP_ALARM_ACTIVATION_EP, &alarm_state);
	}

	if(data->call_pin == 16384){
		read_gpio_state(APP_ALARM_EXT_NORD_EP, &nord_state);
	}

	if(data->call_pin == 8192){
		read_gpio_state(APP_ALARM_EXT_SUD_EP, &sud_state);
	}

	if(data->call_pin == 4096){
		read_gpio_state(APP_ALARM_INT_EP, &internal_state);
	}

	k_msleep(500);
}

void gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	//Create callback struct
	if(pins == 32768){
		gpio_callback_work_with_params_alarm.call_pin = pins;
		k_work_submit(&gpio_callback_work_with_params_alarm.gpio_callback_work);
	}

	if(pins == 16384){
		gpio_callback_work_with_params_nord.call_pin = pins;
		k_work_submit(&gpio_callback_work_with_params_nord.gpio_callback_work);
	}

	if(pins == 8192){
		gpio_callback_work_with_params_sud.call_pin = pins;
		k_work_submit(&gpio_callback_work_with_params_sud.gpio_callback_work);
	}

	if(pins == 4096){
		gpio_callback_work_with_params_int.call_pin = pins;
		k_work_submit(&gpio_callback_work_with_params_int.gpio_callback_work);
	}
	
}

void gpio_callback_init(){

	//Initialize the k_work item used to send zigbee update in gpio interrupt context.
	//This allow execution of routine into callback context (gpio or timer)
    k_work_init(&gpio_callback_work_with_params_alarm.gpio_callback_work, gpio_callback_work_handler);
	k_work_init(&gpio_callback_work_with_params_sud.gpio_callback_work, gpio_callback_work_handler);
	k_work_init(&gpio_callback_work_with_params_nord.gpio_callback_work, gpio_callback_work_handler);
	k_work_init(&gpio_callback_work_with_params_int.gpio_callback_work, gpio_callback_work_handler);
	
	//cb for alarm state
	gpio_pin_interrupt_configure_dt(&alarm_state, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&pin_cb_data_alarm_state, gpio_callback, BIT(alarm_state.pin));
	gpio_add_callback_dt(&alarm_state, &pin_cb_data_alarm_state);

	//cb for nord state
	gpio_pin_interrupt_configure_dt(&nord_state, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&pin_cb_data_nord_state, gpio_callback, BIT(nord_state.pin));
	gpio_add_callback_dt(&nord_state, &pin_cb_data_nord_state);

	//cb for sud state
	gpio_pin_interrupt_configure_dt(&sud_state, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&pin_cb_data_sud_state, gpio_callback, BIT(sud_state.pin));
	gpio_add_callback_dt(&sud_state, &pin_cb_data_sud_state);

	//cb for internal state
	gpio_pin_interrupt_configure_dt(&internal_state, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&pin_cb_data_internal_state, gpio_callback, BIT(internal_state.pin));
	gpio_add_callback_dt(&internal_state, &pin_cb_data_internal_state);
}
	
void timer_callback(struct k_timer *timer_id)
{
	k_work_submit(&timer_callback_work);
}

void timer_callback_work_handler(struct k_work *work){
	LOG_INF("timer_callback");
	battery_report();
	k_msleep(500);
}

void timer_callback_init(){
	//Initialize the k_work item used to send zigbee update in gpio interrupt context.
	//This allow execution of routine into callback context (gpio or timer)
	k_work_init(&timer_callback_work, timer_callback_work_handler);

	//Timer init
	k_timer_init(&timer_interrupt, timer_callback, NULL);
	//it means that timer expires first time after Nsec, after that expires every Nsec
	k_timer_start(&timer_interrupt, K_HOURS(24), K_HOURS(24));  
}

void read_all_states_for_first_time(){
	read_gpio_state(APP_ALARM_ACTIVATION_EP, &alarm_state);
	read_gpio_state(APP_ALARM_EXT_NORD_EP, &nord_state);
	read_gpio_state(APP_ALARM_EXT_SUD_EP, &sud_state);
	read_gpio_state(APP_ALARM_INT_EP, &internal_state);
}

int main(void)
{
	LOG_INF("Starting Zigbee R23 application");

	//GPIO In
	configure_gpio_in(&alarm_state);
	configure_gpio_in(&nord_state);
	configure_gpio_in(&sud_state);
	configure_gpio_in(&internal_state);
	
	//Reset led.
	configure_gpio_out(&led_blue);
	set_inactive_gpio(&led_blue);
	configure_gpio_out(&led_green);
	set_inactive_gpio(&led_green);
	configure_gpio_out(&led_red);
	set_inactive_gpio(&led_red);

	//Config gpio to enable reading battery	
	configure_gpio_out(&enable_voltage_read);
	set_inactive_gpio(&enable_voltage_read);

	//Config adc to read battery
	config_adc_channel();

	/* Initialize */
	//configure_reset_gpio();
	//register_factory_reset_button(FACTORY_RESET_BUTTON);

	/* Register device context (endpoints). */
	ZB_AF_REGISTER_DEVICE_CTX(&app_template_ctx);

	//Init basic cluster attributes
	app_basic_clusters_attr_init();

	//Init binary input attr cluster
	app_binary_input_clusters_attr_init(&alarm_activation, "alarm_activation");
	app_binary_input_clusters_attr_init(&alarm_int, "alarm_int");
	app_binary_input_clusters_attr_init(&alarm_ext_sud, "alarm_ext_sud");
	app_binary_input_clusters_attr_init(&alarm_ext_nord, "alarm_ext_nord");	

	//Init power attr cluster
	app_power_clusters_attr_init();

	/* Register handlers to identify notifications */
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(APP_POWER_EP, identify_cb);
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(APP_ALARM_ACTIVATION_EP, identify_cb);
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(APP_ALARM_INT_EP, identify_cb);
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(APP_ALARM_EXT_SUD_EP, identify_cb);
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(APP_ALARM_EXT_NORD_EP, identify_cb);

	//turn rx off when deep sleep
	zigbee_configure_sleepy_behavior(true);
	
	/* Power off unused sections of RAM to lower device power consumption. */
	if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
		power_down_unused_ram();
	}

	//Start Zigbee default thread
	zigbee_enable();

	LOG_INF("Zigbee R23 application started");

	//Configure and start reporting for power ep
	configureReportingPower(APP_POWER_EP);
	startReportingPower(APP_POWER_EP);

	//Configure and start report binary input eps
	configureReportingAlarm(APP_ALARM_ACTIVATION_EP);
	startReportingAlarm(APP_ALARM_ACTIVATION_EP);

	configureReportingAlarm(APP_ALARM_INT_EP);
	startReportingAlarm(APP_ALARM_INT_EP);

	configureReportingAlarm(APP_ALARM_EXT_SUD_EP);
	startReportingAlarm(APP_ALARM_EXT_SUD_EP);

	configureReportingAlarm(APP_ALARM_EXT_NORD_EP);
	startReportingAlarm(APP_ALARM_EXT_NORD_EP);

	//GPIO callback
	gpio_callback_init();
	
	//Init timer callback
	timer_callback_init();
				
	while(1){
		if(ZB_JOINED()){
			LOG_INF("Zigbee join ok");

			//Read and report state for the first time
			read_all_states_for_first_time();
			battery_report();

			k_sleep(K_FOREVER);			
		}
		
	}
	return 0;
}
