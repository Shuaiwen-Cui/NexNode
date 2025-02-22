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

>DNESP32S3M 模板项目结构

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

!!! NOTE "DNESP32S3M 模板项目结构"
    - `application` 目录下放置应用层代码
    - `middleware` 目录下放置中间件代码
    - `driver` 目录下放置驱动代码
    - `main` 目录下放置主函数代码

## 推荐上手顺序

总之，可以看每个章节里面的依赖关系，保证前置条件满足即可。

0. 项目构建
1. 执行>LED
2. 主控>外部中断
3. 主控>定时器
4. 主控>实时时钟
5. 主控>随机数发生器
6. 主控>SPI & 执行 LCD
7. 主控>SPI & SD 卡

