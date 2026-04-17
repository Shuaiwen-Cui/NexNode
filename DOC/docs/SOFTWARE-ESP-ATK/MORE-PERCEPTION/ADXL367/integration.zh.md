# 集成

## 介绍

!!! note
    在本节中，我们介绍如何将ADXL367驱动集成到项目中。

## 创建新组件

!!! warning
    在 `driver` 目录中创建组件之前，请确保您已将 `driver` 目录合并到项目中，方法是将 `driver` 目录的路径添加到项目级 `CMakeLists.txt` 文件中的 `EXTRA_COMPONENT_DIRS` 变量中。

在 VSCode 中打开项目，打开集成终端，输入

```bash
get_idf 
```

激活 ESP-IDF 环境。然后输入以下命令创建一个名为 `node_acc_adxl367` 的新组件：

```bash
idf.py -C driver create-component node_acc_adxl367
```

上面的命令意味着在 `driver` 目录中创建一个名为 `node_acc_adxl367` 的新组件。执行命令后，您将在 `driver` 目录中看到一个名为 `node_acc_adxl367` 的新目录。命令将自动生成新组件的 `CMakeLists.txt` 文件，以及 `node_acc_adxl367.h` 和 `node_acc_adxl367.c` 文件。

或者，您可以在 `driver` 目录中手动创建 `node_acc_adxl367` 目录，然后在 `node_acc_adxl367` 目录中创建 `CMakeLists.txt`、`node_acc_adxl367.h` 和 `node_acc_adxl367.c` 文件。

在本项目中，在include路径下和ADXL367驱动的目录下各自含有一个test文件，请参考代码部分。

## 替换代码

> 组件层面

将本节中代码分别替换到组件下的`CMakelists.txt`、`node_acc_adxl367.h`和`node_acc_adxl367.c`文件中。

> 项目层面

将`main.c/cpp`中的代码替换为相应的代码。项目层面的`CMakeLists.txt`文件无需更改。

## 编译烧录

在 VSCode 中打开项目，打开集成终端，输入

```bash
idf.py build flash monitor
```

然后您将在串行监视器中看到反馈。

