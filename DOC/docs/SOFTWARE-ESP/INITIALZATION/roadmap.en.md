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