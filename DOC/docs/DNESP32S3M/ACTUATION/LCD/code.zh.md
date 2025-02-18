# 液晶显示屏 代码

## 组件架构

```plaintext
- driver
    - lcd
        - include
            - lcd.h
        - lcd.c
        - CMakeLists.txt
```

## driver/rng/CMakeLists.txt

```cmake
set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
    driver
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
```

!!! note
    注意，在驱动程序中，我们使用了 ESP-IDF 内置的 `driver` 库中的 gpio，因此，我们需要在 `CMakeLists.txt` 文件的 `REQUIRES` 字段中指定此依赖项。

## lcd.h
    
```c


```

## lcd.c

```c

```

## main.c

```c

```