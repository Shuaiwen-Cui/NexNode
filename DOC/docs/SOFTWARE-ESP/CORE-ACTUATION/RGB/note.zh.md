# RGBLED 笔记

## 介绍

!!! note
    本章我们讲解RGBLED灯的驱动和控制。通过修改GPIO引脚，代码也可以用于其他开发板。

## RGBLED灯

![LED](LED.png){ width=800px }

## 电路示意图

![LED_CIRCUIT](LED_CIRCUIT.png){ width=800px }

如图所示，控制LED的GPIO是IO3。

<!-- ## 效果图

![LED-STAT](LED-STAT.jpg){ width=800px } -->

## 依赖关系

LED相关功能依赖ESP-IDF框架中driver/gpio组件，无需额外安装,但是要在项目的CMakeLists.txt中进行声明。

## 关键函数

| 函数原型 | 说明 | 示例 |
| --- | --- | --- |
| `void led_init(void)` | 初始化LED | `led_init();` |
| `void led(int x)` | 控制LED | `led(1);` |
| `void led_toggle(void)` | 切换LED | `led_toggle();` |
