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

1. Execute > LED
2. Main Control > External Interrupt
3. Main Control > Timer
4. Main Control > Real-Time Clock
5. Main Control > Random Number Generator
6. Main Control > SPI & Execute LCD
7. Main Control > SPI & SD Card
8. Communication > WIFI
9. Communication > IOT
10. Main Control > I2C
11. Sensing > ADXL355


## ADDITIONAL NOTES

!!! danger "Additional Notes"
    It is recommended to completely clear the build folder when flashing, and power cycle the development board before flashing.