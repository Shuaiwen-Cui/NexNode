# INTEGRATION

## Introduction

!!! note
    In this section, we introduce how to integrate the DS3231 driver into your project.

## Creating a New Component

!!! warning
    Before creating the component in the `driver` directory, ensure that you have merged the `driver` directory into your project by adding the path of the `driver` directory to the `EXTRA_COMPONENT_DIRS` variable in the project-level `CMakeLists.txt` file.

Open your project in VSCode, open the integrated terminal, and enter

```bash
get_idf
```

to activate the ESP-IDF environment. Then enter the following command to create a new component named `node_rtc_ds3231`:

```bash
idf.py -C driver create-component node_rtc_ds3231
```

The above command means to create a new component named `node_rtc_ds3231` in the `driver` directory. After executing the command, you will see a new directory named `node_rtc_ds3231` in the `driver` directory. The command will automatically generate the `CMakeLists.txt` file for the new component, as well as the `node_rtc_ds3231.h` and `node_rtc_ds3231.c` files.

Alternatively, you can manually create the `node_rtc_ds3231` directory in the `driver` directory, and then create the `CMakeLists.txt`, `node_rtc_ds3231.h`, and `node_rtc_ds3231.c` files in the `node_rtc_ds3231` directory.

## Replacing Code

> At the component level

Replace the code in this section into the `CMakeLists.txt`, `node_rtc_ds3231.h`, and `node_rtc_ds3231.c` files under the component, respectively.

> At the project level

Replace the code in `main.c/cpp` with the corresponding code. There is no need to change the project-level `CMakeLists.txt` file.

## Compiling and Flashing

Open your project in VSCode, open the integrated terminal, and enter

```bash
idf.py build flash monitor
```

Then you will see feedback in the serial monitor. Alternatively, you can check the SD card for verification.