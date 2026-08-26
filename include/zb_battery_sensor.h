/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZB_BATTERY_SENSOR_H
#define ZB_BATTERY_SENSOR_H

/**
 *  @defgroup ZB_DEFINE_DEVICE_BATTERY_SENSOR BATTERY Sensor
 *  @{
 *  @details
 *      - @ref ZB_ZCL_BASIC \n
 *      - @ref ZB_ZCL_IDENTIFY \n
 *      - @ref ZB_ZCL_BINARY_INPUT
 */

/** BATTERY Sensor Device ID */
#define ZB_BATTERY_SENSOR_DEVICE_ID 0x0402

/** BATTERY Sensor device version */
#define ZB_DEVICE_VER_BATTERY_SENSOR 0

/** @cond internals_doc */

/** BATTERY Sensor IN (server) clusters number */
#define ZB_BATTERY_SENSOR_IN_CLUSTER_NUM 3

/** BATTERY Sensor OUT (client) clusters number */
#define ZB_BATTERY_SENSOR_OUT_CLUSTER_NUM 0

/** BATTERY Sensor total (IN+OUT) cluster number */
#define ZB_BATTERY_SENSOR_CLUSTER_NUM \
	(ZB_BATTERY_SENSOR_IN_CLUSTER_NUM + ZB_BATTERY_SENSOR_OUT_CLUSTER_NUM)

/** Number of attribute for reporting on BATTERY Sensor device */
#define ZB_BATTERY_SENSOR_REPORT_ATTR_COUNT ZB_ZCL_ANALOG_INPUT_REPORT_ATTR_COUNT

/** @endcond */ /* internals_doc */

/**
 * @brief Declare cluster list for BATTERY Sensor device
 * @param cluster_list_name - cluster list variable name
 * @param basic_server_attr_list - attribute list for Basic cluster (server role)
 * @param identify_server_attr_list - attribute list for Identify cluster (server role)
 * @param analog_input_attr_list - attribute list for binary_input cluster (server role)
 */
#define ZB_DECLARE_BATTERY_SENSOR_CLUSTER_LIST(					  \
		cluster_list_name,						  \
		basic_server_attr_list,						  \
		identify_server_attr_list,					  \
		analog_input_attr_list)					  \
zb_zcl_cluster_desc_t cluster_list_name[] =					  \
{										  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_BASIC,					  \
		ZB_ZCL_ARRAY_SIZE(basic_server_attr_list, zb_zcl_attr_t),	  \
		(basic_server_attr_list),					  \
		ZB_ZCL_CLUSTER_SERVER_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	),									  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_IDENTIFY,					  \
		ZB_ZCL_ARRAY_SIZE(identify_server_attr_list, zb_zcl_attr_t),	  \
		(identify_server_attr_list),					  \
		ZB_ZCL_CLUSTER_SERVER_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	),									  \
	ZB_ZCL_CLUSTER_DESC(							  \
		ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,					  \
		ZB_ZCL_ARRAY_SIZE(analog_input_attr_list, zb_zcl_attr_t),	  \
		(analog_input_attr_list),					  \
		ZB_ZCL_CLUSTER_SERVER_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	)									  \
}


/** @cond internals_doc */
/**
 * @brief Declare simple descriptor for BATTERY Sensor device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param in_clust_num - number of supported input clusters
 * @param out_clust_num - number of supported output clusters
 */
#define ZB_ZCL_DECLARE_BATTERY_SENSOR_SIMPLE_DESC(				    \
	ep_name, ep_id, in_clust_num, out_clust_num)				    \
	ZB_DECLARE_SIMPLE_DESC_VA(in_clust_num, out_clust_num, ep_name);			    \
	ZB_AF_SIMPLE_DESC_TYPE_VA(in_clust_num, out_clust_num, ep_name) simple_desc_##ep_name = \
	{									    \
		ep_id,								    \
		ZB_AF_HA_PROFILE_ID,						    \
		ZB_BATTERY_SENSOR_DEVICE_ID,					    \
		ZB_DEVICE_VER_BATTERY_SENSOR,					    \
		0,								    \
		in_clust_num,							    \
		out_clust_num,							    \
		{								    \
			ZB_ZCL_CLUSTER_ID_BASIC,				    \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,				    \
			ZB_ZCL_CLUSTER_ID_ANALOG_INPUT				    \
		}								    \
	}

/** @endcond */ /* internals_doc */

/**
 * @brief Declare endpoint for BATTERY Sensor device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param cluster_list - endpoint cluster list
 * @param reporting_ctx
 */
#define ZB_DECLARE_BATTERY_SENSOR_EP(ep_name, ep_id, cluster_list)		      \
	ZB_ZCL_DECLARE_BATTERY_SENSOR_SIMPLE_DESC(ep_name, ep_id,		      \
		ZB_BATTERY_SENSOR_IN_CLUSTER_NUM, ZB_BATTERY_SENSOR_OUT_CLUSTER_NUM); \
	ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_info##ep_name,		      \
		ZB_BATTERY_SENSOR_REPORT_ATTR_COUNT);  \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,     \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
		(zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,		      \
		ZB_BATTERY_SENSOR_REPORT_ATTR_COUNT, \
		reporting_info##ep_name, /* Use reporting ctx to transmit attribute */    \
		0, NULL) /* No CVC ctx */

/** @} */

#endif /* ZB_BATTERY_SENSOR_H */