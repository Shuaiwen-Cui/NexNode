# INTEGRATION

## Introduction

!!! note
    In this section, we introduce how to integrate the wifi driver into the project.

!!! danger
    For testing convenience in campus network environments, the wifi used in this project is enterprise WiFi, which requires WPA2 Enterprise mode and PEAP-MSCHAPV2 authentication. If you need to connect to a regular WiFi network (such as home WiFi), please implement it separately.

## Create a New Component

!!! warning
    Before you create a component in the `driver` directory, make sure you have incorporated the `driver` directory into the project by adding the path to the `driver` directory to the `EXTRA_COMPONENT_DIRS` variable in the project-level `CMakeLists.txt` file.

Open the project in VSCode, open the integrated terminal, and type in

```bash
get_idf 
```
to actiate the ESP-IDF environment. Then type in the following command to create a new component named `node_wifi`:

```bash
idf.py -C driver create-component node_wifi
```

The command above means to create a new component named `node_wifi` in the `driver` directory. After executing the command, you will see a new directory named `node_wifi` in the `driver` directory. And the command will automatically generate the `CMakeLists.txt` file for the new component, as well as the `node_wifi.h` and `node_wifi.c` files.

Alternatively, you can mannually create the `node_wifi` directory in the `driver` directory, and then create the `CMakeLists.txt`, `node_wifi.h`, and `node_wifi.c` files in the `node_wifi` directory.

## Replace the Code

> Component Level

Replace the code in this section into the `CMakelists.txt`, `node_wifi.h`, and `node_wifi.c` files under the component.

> Project Level

Replace the code in `main.c/cpp` with the corresponding code. No need to change the `CMakeLists.txt` file at the project level.

## Compile and Flash the Project

Open the project in VSCode, open the integrated terminal, and type in

```bash
idf.py build flash monitor
```

Now, you will see random numbers from the serial monitor.
