# Xiao nRF52840 + nRF Sdk Connect + Zigbee R23 🚀
The target of this project is to develop a Zigbee End Device using Xiao nRF52840 SoC.

Below i exaplain and describe all i've done to create them.

## Software requirements
1) Develop sw for nRF SoC require [nRF SDK Connect](https://nrfconnectdocs.nordicsemi.com/ncs/latest/nrf/index.html) which provide all libraries (basically Zephyr) and evironment to develop and flash applications.

   *  In my case i used VS Code extention which provide all you need to develop.
    [Here](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-VS-Code/Tutorials?lang=en#infotabs) a tutorial i follow to install nRF SDK Connect via VS Code.
   
   * Others documents like [this](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/lessons/lesson-1-nrf-connect-sdk-introduction/topic/exercise-1-1/) explain how to install and configure nRF SDK Connect         extention.

2) To develop Zigbee device, nRF SDK Connect require [Zigbee R23 Add On](https://nrfconnectdocs.nordicsemi.com/addons/zigbee-r23/latest/index.html#), basically it implement the ZBOSS Zigbee stack.
[Here](https://nrfconnectdocs.nordicsemi.com/addons/ncs-zigbee/latest/setup.html#software-requirements) the documentation to get them. Simply this procedure clone a git repository (workspace) in which you can develop Zigbee applications.

   * With VS Code enter in root af this worksapace (be carfully to this so, otherwise the application build procedure will miss the essentials libraries).
   * Create new application and serch by "ncs", it will display all examples, and choose one.
   * Add build configuration like this: show image . In the type of build choose "no sysbuild", this permit to create .uf2 build file suitable for Xiao nRF52840 default bootloader. Make sure to have in the project the file `pm_static.yml` which define the memory map of firmware that will be installed.    
   * After that you can run build process.

3) Flash device.
   
   Be carefully that Xiao nRF52840 board uses UF2 default bootloader, that contrary to McuBoot (default for nRF SDK Connect) allows to load firmware by drag and drop.
   So, after the build, double click on Xiao nRF52840 reset button to enter in boot mode (showing chip like a hard drive) and drag and drop .uf2 file. After that SoC will restart automatically.
       

## Hardware requirements

### readme under construction... 🚧🛠️⏳🔄🔜
