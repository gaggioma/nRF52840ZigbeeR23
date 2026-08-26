#ifndef ZB_POWER_H
#define ZB_POWER_H

/**
 *  @defgroup ZB_DEFINE_DEVICE_POWER Alarm Sensor
 *  @{
 *  @details
 *      - @ref ZB_ZCL_BASIC \n
 *      - @ref ZB_ZCL_IDENTIFY \n
 *      - @ref ZB_ZCL_BINARY_INPUT
 */

/** Alarm Sensor Device ID */
#define ZB_POWER_DEVICE_ID 0x0402

/** Alarm Sensor device version */
#define ZB_DEVICE_VER_POWER 0

/** @cond internals_doc */

/** Sensor IN (server) clusters number = basic + info + power*/
#define ZB_POWER_IN_CLUSTER_NUM 3

/** Sensor OUT (client) clusters number */
#define ZB_POWER_OUT_CLUSTER_NUM 0

/** Sensor total (IN+OUT) cluster number */
#define ZB_POWER_CLUSTER_NUM \
	(ZB_POWER_IN_CLUSTER_NUM + ZB_POWER_OUT_CLUSTER_NUM)

/** Number of attribute for reporting: defalut (alarm state + percent) + voltage */
#define POWER_REPORT_ATTR_COUNT \
	(ZB_ZCL_POWER_CONFIG_REPORT_ATTR_COUNT + 1)


//Set VOLTAGE_ID reportable
#ifdef ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID
#undef ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID
#endif

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID(data_ptr, bat_num) \
{                                                               \
  ZB_ZCL_ATTR_POWER_CONFIG_BATTERY##bat_num##_VOLTAGE_ID,       \
  ZB_ZCL_ATTR_TYPE_U8,                                          \
  ZB_ZCL_ATTR_ACCESS_READ_ONLY  | ZB_ZCL_ATTR_ACCESS_REPORTING,                                 \
  (ZB_ZCL_NON_MANUFACTURER_SPECIFIC),                           \
  (void*) data_ptr                                              \
}

/**
 * @brief Declare cluster list for Alarm Sensor device
 * @param cluster_list_name - cluster list variable name
 * @param basic_server_attr_list - attribute list for Basic cluster (server role)
 * @param identify_server_attr_list - attribute list for Identify cluster (server role)
 * @param binary_input_attr_list - attribute list for binary_input cluster (server role)
 */
#define ZB_DECLARE_POWER_CLUSTER_LIST(					  \
		cluster_list_name,						  \
		basic_server_attr_list,						  \
		identify_server_attr_list,					  \
		power_attr_list)					  \
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
		ZB_ZCL_CLUSTER_ID_POWER_CONFIG,					  \
		ZB_ZCL_ARRAY_SIZE(power_attr_list, zb_zcl_attr_t),	  \
		(power_attr_list),					  \
		ZB_ZCL_CLUSTER_SERVER_ROLE,					  \
		ZB_ZCL_MANUF_CODE_INVALID					  \
	)									  \
}


/** @cond internals_doc */
/**
 * @brief Declare simple descriptor for Alarm Sensor device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param in_clust_num - number of supported input clusters
 * @param out_clust_num - number of supported output clusters
 */
#define ZB_ZCL_DECLARE_POWER_SIMPLE_DESC(				    \
	ep_name, ep_id, in_clust_num, out_clust_num)				    \
	ZB_DECLARE_SIMPLE_DESC_VA(in_clust_num, out_clust_num, ep_name);			    \
	ZB_AF_SIMPLE_DESC_TYPE_VA(in_clust_num, out_clust_num, ep_name) simple_desc_##ep_name = \
	{									    \
		ep_id,								    \
		ZB_AF_HA_PROFILE_ID,						    \
		ZB_POWER_DEVICE_ID,					    \
		ZB_DEVICE_VER_POWER,					    \
		0,								    \
		in_clust_num,							    \
		out_clust_num,							    \
		{								    \
			ZB_ZCL_CLUSTER_ID_BASIC,				    \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,				    \
			ZB_ZCL_CLUSTER_ID_POWER_CONFIG 			    \
		}								    \
	}

/** @endcond */ /* internals_doc */

/**
 * @brief Declare endpoint for Alarm Sensor device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param cluster_list - endpoint cluster list
 */
#define ZB_DECLARE_POWER_EP(ep_name, ep_id, cluster_list)		      \
	ZB_ZCL_DECLARE_POWER_SIMPLE_DESC(ep_name, ep_id,		      \
		  ZB_POWER_IN_CLUSTER_NUM, ZB_POWER_OUT_CLUSTER_NUM); \
	ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_info##ep_name,		      \
		POWER_REPORT_ATTR_COUNT);				      \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,     \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
		(zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,		      \
		POWER_REPORT_ATTR_COUNT, \
		reporting_info##ep_name, /* Use reporting ctx to transmit attribute */    \
		0, NULL) /* No CVC ctx */

/** @} */

#endif /* ZB_POWER_H */
