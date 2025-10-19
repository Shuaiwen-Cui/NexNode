# 核心功能开发

!!! info "简介"
    在初始化一章中，我们已经介绍了如何搭建开发环境以及基本的项目结构。本章将在此基础上，将项目代码进行扩展，打造具备核心功能的物联网节点代码框架。

## 项目框架改造

之前的版本只是最基础的“Hello World”示例代码，无法满足实际应用需求。对代码进行层次化和模块化管理是大型项目开发的基础。在项目层级，我们增设与`main`文件夹同级的`driver`,`middleware`,`application`文件夹，分别用于存放硬件驱动代码、中间件代码和应用层代码。新增加的这三个文件夹里面我们各自放入一个readme.txt文件，以避免空文件夹无法被Git管理的问题。效果如图所示：

![](PROJ-ARCH.png){width="250"}

在执行完以上操作后，我们还需要对`CMakeLists.txt`文件进行修改，以确保新增加的文件夹能够被正确编译。修改后的项目层面的`CMakeLists.txt`文件内容如下所示：

```cmake
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "./driver" "./middleware" "./application")

project(AIoTNode)
```

!!! warning "注意"
    请确保`EXTRA_COMPONENT_DIRS`变量中包含了新增加的文件夹路径，否则编译器将无法找到这些组件，导致编译失败。

!!! tip "建议"
    本项目的核心目标是打造传感器节点，即硬件设计和基础的驱动实现，其实并不会过多涉及中间件和应用层的开发内容。但是为了让项目结构更加完整和规范，并为后续项目提供基础，这里我们依然创建了这中间件和应用层的文件夹。

## 版本说明

通过同时对AIOT-C-ZERO和AIOT-CPP-ZERO版本进行以上改造，我们可以得到版本AIOT-C-CORE和AIOT-CPP-CORE的项目框架，后续会基于这两个版本进行核心功能的开发。