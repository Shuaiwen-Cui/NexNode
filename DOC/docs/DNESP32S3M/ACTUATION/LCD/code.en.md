# LCD CODE

## Component Architecture

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
    Note that in the drivers, we used gpio from the ESP-IDF builtin `driver` library, therefore, we need to indicate this dependency in the `REQUIRES` field of the `CMakeLists.txt` file. 

## lcd.h
    
```c


```

## lcd.c

```c

```

## main.c

```c

```