# 正点原子 DNESP32S3M 开发板

## 硬件架构

!!! NOTE "硬件架构"
    主控 + 感知 + 通信 + 执行 + 电源
 
>正点原子 DNESP32S3M 主控板

![DNESP32S3M](DNESP32S3M.png){width=800px}

![DNESP32S3M-BACK](DNESP32S3M-BACK.png){width=800px}

## 代码架构

>一般意义上的 ESP32 项目结构

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

>DNESP32S3M 初始项目结构

```txt
- DNESP32S3M-INIT/
             - .vscode/ (vscode 设置文件，可以忽略)
             - CMakeLists.txt (项目级 cmake 文件)
             - sdkconfig (项目级 sdkconfig 文件，通过 idf.py menuconfig 生成)
             - sdkconfig.old (sdkconfig 备份)
             - partitions-16MiB.csv (16MiB flash 调整的分区表文件)
             - application/ - component1/ - CMakeLists.txt
                                          - Kconfig
                                          - src1.c
                            - component2/ - CMakeLists.txt
                                          - Kconfig
                                          - src1.c
                                          - include/ - component2.h
             - middleware/  - component1/ - CMakeLists.txt
                                          - Kconfig
                                          - src1.c
                            - component2/ - CMakeLists.txt
                                          - Kconfig
                                          - src1.c
                                          - include/ - component2.h
             - driver/      - component1/ - CMakeLists.txt
                                          - Kconfig
                                          - src1.c
                            - component2/ - CMakeLists.txt
                                          - Kconfig
                                          - src1.c
                                          - include/ - component2.h
             - main/        - CMakeLists.txt
                            - src1.c
                            - src2.c
             - build/

```

🏗️