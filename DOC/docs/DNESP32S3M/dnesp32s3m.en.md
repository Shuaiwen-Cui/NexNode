# ALIENTEK DNESP32S3M

## HARDWARE ARCHITECTURE

!!! NOTE "Hardware Architecture"
    MAIN CONTROL + PERCEPTION + COMMUNICATION + ACTUATION + POWER

>Alientek DNESP32S3M Main Control Board

![DNESP32S3M](DNESP32S3M.png){width=800px}

![DNESP32S3M-BACK](DNESP32S3M-BACK.png){width=800px}

## CODE ARCHITECTURE

>General ESP32 Project Structure

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
>DNESP32S3M TEMPLATE PROJECT STRUCTURE

```txt
- DNESP32S3M-XXX/
             - .vscode/ (vscode 设置文件，可以忽略)
             - CMakeLists.txt (项目级 cmake 文件)
             - sdkconfig (项目级 sdkconfig 文件，通过 idf.py menuconfig 生成)
             - sdkconfig.old (sdkconfig 备份)
             - partitions-16MiB.csv (16MiB flash 调整的分区表文件)
             - dependencies.lock (依赖库版本锁定文件)
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

!!! NOTE "CODE ARCHITECTURE"
    - application: Application layer, including the main program and the application layer components.
    - middleware: Middleware layer, including middleware components.
    - driver: Driver layer, including driver components.
    - main: Main program, including the main program and the main program components.

## RECOMMENDED ORDER

In short, you can refer to the dependency relationship in each chapter. Ensure all the prerequisites are met before proceeding to the current chapter.

0. PROJECT CONSTRRUCTION
1. ACTUATION>LED
2. MAIN-CONTROL>EXIT (External Interrupt)
3. MAIN-CONTROL>TIMER
4. MAIN-CONTROL>RTC
5. MAIN-CONTROL>RNG
6. MAIN-CONTROL>SPI & ACTUATION LCD
7. MAIN-CONTROL>SPI & SD-CARD

