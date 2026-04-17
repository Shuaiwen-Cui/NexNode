# 集成

## 介绍

!!! note
    在本节中，我们介绍如何将mqtt驱动集成到项目中。

## 创建新组件

!!! warning
    在 `driver` 目录中创建组件之前，请确保您已将 `driver` 目录合并到项目中，方法是将 `driver` 目录的路径添加到项目级 `CMakeLists.txt` 文件中的 `EXTRA_COMPONENT_DIRS` 变量中。

在 VSCode 中打开项目，打开集成终端，输入

```bash
get_idf 
```

激活 ESP-IDF 环境。然后输入以下命令创建一个名为 `node_mqtt` 的新组件：

```bash
idf.py -C driver create-component node_mqtt
```

!!! danger
    注意，esp-idf里有内置的mqtt模块，因此为了避免冲突，我们将这个模块命名为node_mqtt。以前ESPRESSIF是内置mqtt模块的，但后来ESPRESSIF将mqtt模块从内置模块中剥离出来了，因此现在ESP-IDF里没有内置mqtt模块了。我们的node_mqtt模块还是依赖这个库，但是这个库已经不属于ESP-IDF的内置模块了，现在要想使用要么使用在线下载的方式，要么使用离线包的方式。我们这里使用离线包的方式，我已经把这个库放在driver目录下了，请确保这个包也在你的项目driver目录下。

上面的命令意味着在 `driver` 目录中创建一个名为 `node_mqtt` 的新组件。执行命令后，您将在 `driver` 目录中看到一个名为 `node_mqtt` 的新目录。命令将自动生成新组件的 `CMakeLists.txt` 文件，以及 `node_mqtt.h` 和 `node_mqtt.c` 文件。

或者，您可以在 `driver` 目录中手动创建 `node_mqtt` 目录，然后在 `node_mqtt` 目录中创建 `CMakeLists.txt`、`node_mqtt.h` 和 `node_mqtt.c` 文件。

## 替换代码

> 组件层面

将本节中代码分别替换到组件下的`CMakelists.txt`、`node_mqtt.h`和`node_mqtt.c`文件中。

> 项目层面

将`main.c`中的代码替换为相应的代码。项目层面的`CMakeLists.txt`文件无需更改。

## 编译烧录

在 VSCode 中打开项目，打开集成终端，输入

```bash
idf.py build flash monitor
```
此时，您将在终端看到相应的输出信息。