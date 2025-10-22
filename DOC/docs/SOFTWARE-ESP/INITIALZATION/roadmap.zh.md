# 开发路线图

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

>AIoTNode 模板项目结构

```txt
- AIoTNode/
             - .vscode/ (vscode 设置文件，可以忽略)
             - CMakeLists.txt (项目级 cmake 文件)
             - sdkconfig (项目级 sdkconfig 文件，通过 idf.py menuconfig 生成)
             - sdkconfig.old (sdkconfig 备份)
             - partitions-16MiB.csv (16MiB flash 调整的分区表文件)
             - dependencies.lock (依赖库版本锁定文件,自动生成，首次运行前请删除此文件)
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

!!! NOTE "AIoTNode 模板项目结构"
    - `application` 目录下放置应用层代码
    - `middleware` 目录下放置中间件代码
    - `driver` 目录下放置驱动代码
    - `main` 目录下放置主函数代码

## 本项目开发顺序

!!! tip
    对于每个组件，需要保证其前置条件已经满足，才能进行后续的开发。

假定您已完成项目初始化工作，建议按照以下顺序进行各个模块的开发：

| 功能类别             | 功能模块         | 依赖组件（Official）     | 依赖组件（User）    |
|----------------------|------------------|--------------------------|---------------------|
| 执行                 | LED              | driver                     |                     |
| 控制                 | 外部中断          | driver                     |   node_led                  |
| 控制                 | 定时器            | driver                     |   node_led                  |
| 控制                 | 实时时钟          |                     |                     |
| 控制                 | 随机数发生器      |                      |                     |
| 控制                 | SPI              | driver                     |                     |
| 执行                 | LCD              | driver                     | node_spi                     |
| 执行                 | SD 卡            | driver, fatfs, vfs, sdmmc   | node_spi                     |
| 通信                 | WIFI             | esp_wifi, wpa_supplicant    | node_led, node_lcd           |


1. 执行>LED
2. 主控>外部中断
3. 主控>定时器
4. 主控>实时时钟
5. 主控>随机数发生器
6. 主控>SPI & 执行 LCD
7. 主控>SPI & SD 卡
8. 通信>WIFI
9. 通信>IOT
10. 主控>I2C
11. 感知>ADXL355

## 额外说明

!!! danger "额外说明"
    建议烧录的时候，彻底清空build文件夹，开发板重新上电后烧录。