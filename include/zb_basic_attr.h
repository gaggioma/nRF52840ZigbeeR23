// Basic cluster attributes initial values. For more information, see section 3.2.2.2 of the ZCL specification.
#define BASIC_APP_VERSION       01                                  // Version of the application software (1 byte).
#define BASIC_STACK_VERSION     10                                  // Version of the implementation of the Zigbee stack (1 byte).
#define BASIC_HW_VERSION        11                                  // Version of the hardware of the device (1 byte).
#define BASIC_MANUF_NAME        "Marco Corp."                     // Manufacturer name (32 bytes).
#define BASIC_MODEL_ID          "nRF"                        // Model number assigned by the manufacturer (32-bytes long string).
#define BASIC_DATE_CODE         "20260810"                          // Date provided by the manufacturer of the device in ISO 8601 format (YYYYMMDD), for the first 8 bytes. The remaining 8 bytes are manufacturer-specific.
#define BASIC_POWER_SOURCE      ZB_ZCL_BASIC_POWER_SOURCE_BATTERY   // Type of power source or sources available for the device. For possible values, see section 3.2.2.2.8 of the ZCL specification.
#define BASIC_LOCATION_DESC     "Earth"                             // Description of the physical location of the device (16 bytes). You can modify it during the commisioning process.
#define BASIC_PH_ENV            ZB_ZCL_BASIC_ENV_UNSPECIFIED        // Description of the type of physical environment. For possible values, see section 3.2.2.2.10 of the ZCL specification.
