# DEVELOPMENT ROADMAP


## CODE STRUCTURE

> GENERIC ESP32 PROJECT STRUCTURE

```txt
- myProject/
             - CMakeLists.txt
             - sdkconfig
             - components/ - component1/ - CMakeLists.txt
                                         - Kconfig
                                         - src1.c
                           - component2/ - CMakeLists.txt
                                         - Kconfig
                                         - src1.c
                                         - include/ - component2.h
             - main/       - CMakeLists.txt
                           - src1.c
                           - src2.c
             - build/

```

> AIoTNode TEMPLATE PROJECT STRUCTURE

```txt
- AIoTNode/
             - .vscode/ (vscode configuration files, can be ignored)
             - CMakeLists.txt (project-level cmake file)
             - sdkconfig (project-level sdkconfig file, generated via idf.py menuconfig)
             - sdkconfig.old (sdkconfig backup)
             - partitions-16MiB.csv (16MiB flash partition table file)
             - dependencies.lock (dependency library version lock file, automatically generated, please delete this file before the first run)
             - application/         - component1/ - CMakeLists.txt
                                                  - Kconfig
                                                  - src1.c
                                    - component2/ - CMakeLists.txt
                                                  - Kconfig
                                                  - src1.c
                                                  - include/ - component2.h
             - middleware/          - component1/ - CMakeLists.txt
                                                  - Kconfig
                                                  - src1.c
                                    - component2/ - CMakeLists.txt
                                                  - Kconfig
                                                  - src1.c
                                                  - include/ - component2.h
             - driver/              - component1/ - CMakeLists.txt
                                                  - Kconfig
                                                  - src1.c
                                    - component2/ - CMakeLists.txt
                                                  - Kconfig
                                                  - src1.c
                                                  - include/ - component2.h
             - main/                - CMakeLists.txt
                                    - src1.c
                                    - src2.c
             - build/

```

!!! NOTE "AIoTNode TEMPLATE PROJECT STRUCTURE"
    - `application` directory is for application layer code
    - `middleware` directory is for middleware code
    - `driver` directory is for driver code
    - `main` directory is for main function code

Ontop of the ZERO template, we have added `application`, `middleware`, and `driver` directories to better organize the code structure. Based on different functionalities, we categorize the code into application layer, middleware layer, and driver layer to improve code maintainability and scalability.

Correspondingly, we need to modify the entire project's `CMakeLists.txt` file to correctly include the code in these directories. Specifically, we need to overwrite the `CMakeLists.txt` with the following content to ensure that the newly added directories can be correctly compiled:

```cmake
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# Extra components (e.g. driver/node_led): one subfolder with CMakeLists.txt = one component.
# With no REQUIRES in main/CMakeLists.txt, `main` depends on all built components automatically.
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/driver" "${CMAKE_SOURCE_DIR}/middleware" "${CMAKE_SOURCE_DIR}/application")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(AIoTNode)
```

In case git does not synchronize empty folders, we have placed an empty `readme.txt` file in each directory. Users can delete this file and add their own code in the corresponding directory.

## DEVELOPMENT SEQUENCE

!!! tip
    For each component, ensure that its prerequisites are met before proceeding with subsequent development.

Assuming you have completed the project initialization work, it is recommended to develop each module in the following order:

| Function Category    | Function Module  | Dependency Component (Official) | Dependency Component (User) |
|----------------------|------------------|----------------------------------|-----------------------------|
| Actuation            | LED              | driver                           |                             |
| Actuation            | External Interrupt| driver                           |   node_led                  |
| Main Control        | Timer            | driver                           |   node_led                  |
| Main Control        | Real-Time Clock  |                                  |                             |
| Main Control        | Random Number Generator|                              |                             |
| Main Control        | I2C              | driver                           |                             |
| Main Control        | SPI              | driver                           |                             |
| Actuation            | LCD              | driver                           | node_spi                    |
| Actuation            | SD Card          | driver, fatfs, vfs, sdmmc     | node_spi                    |
| Communication       | WIFI             | esp_wifi, wpa_supplicant    | node_led, node_lcd(optional)           |
| Communication       | IOT              | mqtt     |            |
| Other                |                  |                             |                              |

## ADDITIONAL NOTES

!!! danger "Additional Notes"
    It is recommended to completely clear the build folder when flashing, and power cycle the development board before flashing.