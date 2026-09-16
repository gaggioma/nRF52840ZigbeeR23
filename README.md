# Xiao nRF52840 + nRF Sdk Connect + Zigbee R23 + Zigbee2MQTT 🚀
The target of this project is to develop a Zigbee End Device using Xiao nRF52840 SoC, exposing battery percentage and battery voltage attribute.

Below i explain and describe all i've done to create them.

## Software requirements
1) Develop sw for nRF SoC require [nRF SDK Connect](https://nrfconnectdocs.nordicsemi.com/ncs/latest/nrf/index.html) which provide all libraries (basically Zephyr) and evironment to develop and flash applications.

   *  In my case i used VS Code extention which provide all you need to develop.
    [Here](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-VS-Code/Tutorials?lang=en#infotabs) a tutorial i follow to install nRF SDK Connect via VS Code.
   
   * Others documents like [this](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-1-nrf-connect-sdk-introduction/topic/exercise-1-1/) explain how to install and configure nRF SDK Connect         extention.

2) To develop Zigbee device:
  
   nRF SDK Connect require [Zigbee R23 Add On](https://nrfconnectdocs.nordicsemi.com/addons/zigbee-r23/latest/index.html#), basically it implement the ZBOSS Zigbee stack.
   [Here](https://nrfconnectdocs.nordicsemi.com/addons/ncs-zigbee/latest/setup.html#software-requirements) the documentation to get them. This procedure clone a git repository (workspace) in which you can develop       Zigbee applications. [Here](https://ncsdoc.z6.web.core.windows.net/zboss-r23/4.2.2.4/index.html) you can find all APIs used in Zigbee stack.

   * With VS Code enter in root af this worksapace (be carefully to this so, otherwise samples will not be found).
   * Click create new application -> Copy a sample -> select SDK version "ncs-zigbee" -> search "ncs-zigbee", it will display all examples, and choose one. Save the new project into the root of workspace, otherwise the application build procedure will miss the essentials libraries.
   * Add build configuration like this:
    <picture>
      <img src="/images/build_1.PNG" alt="build_conf" style="width:auto;">
    </picture>

    In the type of build choose "no sysbuild", this permit to create .uf2 build file suitable for Xiao nRF52840 default bootloader.
    Make sure to have in the project the file `pm_static.yml` which define the memory map of firmware that will be installed.    
   * After that you can run build process.

4) Flash device.
   
   Be carefully that Xiao nRF52840 board uses UF2 default bootloader, that differently from McuBoot (default for nRF SDK Connect) allows to load firmware by drag and drop.
   So, after the build, double click on Xiao nRF52840 reset button to enter in boot mode (pc shows chip like a hard drive) and drag and drop `.uf2` file. After that, SoC will restart automatically.
      

## Hardware requirements
For this project i used Xiao nRF52840 buyed [here](https://it.aliexpress.com/item/1005006988954136.html?spm=a2g0o.order_list.order_list_main.17.5cf21802FKmGBO&gatewayAdapt=glo2ita).

For all pinouts and hw specs i used [this](https://wiki.seeedstudio.com/XIAO_BLE/). 

## Zigbee clusters tips
In this device i used 2 clusters definition:
 1) [Binary Input](https://ncsdoc.z6.web.core.windows.net/zboss-r23/4.2.2.4/group___z_b___z_c_l___b_i_n_a_r_y___i_n_p_u_t.html);
 2) [Power Configuration](https://ncsdoc.z6.web.core.windows.net/zboss-r23/4.2.2.4/group___z_b___z_c_l___p_o_w_e_r___c_o_n_f_i_g.html).

In Binary Input cluster definition, i modified the attributes assignment to allow the definition of DESCRIPTION field like this:
```
#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_ATTR_BINARY_INPUT_DESCRIPTION_ID(data_ptr) \
{                                                                   \
  ZB_ZCL_ATTR_BINARY_INPUT_DESCRIPTION_ID,                          \
  ZB_ZCL_ATTR_TYPE_CHAR_STRING,                                     \
  ZB_ZCL_ATTR_ACCESS_READ_ONLY | ZB_ZCL_ATTR_ACCESS_WRITE_OPTIONAL, \
  (ZB_ZCL_NON_MANUFACTURER_SPECIFIC),                               \
  (void*) data_ptr                                                  \
}

#define ZB_ZCL_DECLARE_BINARY_INPUT_ATTRIB_LIST_EXT(                                     \
    attr_list, out_of_service, present_value, status_flag, description)                           \
  ZB_ZCL_START_DECLARE_ATTRIB_LIST_CLUSTER_REVISION(attr_list, ZB_ZCL_BINARY_INPUT)  \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_BINARY_INPUT_OUT_OF_SERVICE_ID, (out_of_service)) \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_BINARY_INPUT_PRESENT_VALUE_ID, (present_value))   \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_BINARY_INPUT_STATUS_FLAG_ID, (status_flag))       \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_BINARY_INPUT_DESCRIPTION_ID, (description))       \
  ZB_ZCL_FINISH_DECLARE_ATTRIB_LIST
```

In the Power Configuration cluster, by default the battery voltage field is not set like REPORTABLE, so i modified the cluster definition like this:
```
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
```

## Zigbee2MQTT config
I noticed that ony binary clusters are configured to show up by default. But clusters like Power Config are not configured in that way.

To see all clusters of my device in Zigbee2MQTT console, an [external converter](https://www.zigbee2mqtt.io/advanced/more/external_converters.html) is mandatory.
In zigbee2MQTT folder you can find the code used in my project.

## Power optimization
Zigbee end devices are notable for their vey low power consumption.
To obtain this i applied these suggestions found in documentation:
- [Sleepy end device configuration](https://nrfconnectdocs.nordicsemi.com/addons/ncs-zigbee/latest/configuring.html#sleepy-end-device-behavior);
- [Power saving during sleep](https://nrfconnectdocs.nordicsemi.com/addons/ncs-zigbee/latest/configuring.html#power-saving-during-sleep);
- [Power management module](https://nrfconnectdocs.nordicsemi.com/ncs/latest/nrf/test_and_optimize/optimizing/power_general.html#enable-device-power-management-module) to shutdown unused drivers.


### readme under construction... 🚧🛠️⏳🔄🔜
