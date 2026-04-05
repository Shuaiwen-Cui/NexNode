# CORE FUNCTION DEVELOPMENT

!!! info "Introduction"
    In the initialization chapter, we have introduced how to set up the development environment and the basic project structure. This chapter will build upon that foundation to expand the project code and create a code framework for an IoT node with core functionalities.

!!! info "Core Functionalities"
    Core functionalities refer to those that can be achieved with just the core board, without the need for additional peripheral circuits or modules.

## PROJECT FRAMEWORK MODIFICATION

The previous version was just the most basic "Hello World" example code, which could not meet the needs of practical applications. Hierarchical and modular management of code is the foundation for large-scale project development. At the project level, we added `driver`, `middleware`, and `application` folders at the same level as the `main` folder to store hardware driver code, middleware code, and application layer code, respectively. Each of these newly added folders contains a readme.txt file to avoid issues with empty folders not being managed by Git. The structure is shown in the figure below:

![](PROJ-ARCH.png){width="250"}

After completing the above operations, we also need to modify the `CMakeLists.txt` file to ensure that the newly added folders can be correctly compiled. The modified project-level `CMakeLists.txt` file content is as follows:

```cmake
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# Extra components (e.g. driver/node_led): one subfolder with CMakeLists.txt = one component.
# With no REQUIRES in main/CMakeLists.txt, `main` depends on all built components automatically.
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/driver" "${CMAKE_SOURCE_DIR}/middleware" "${CMAKE_SOURCE_DIR}/application")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(AIoTNode)
```

!!! warning "Note"
    Please ensure that the `EXTRA_COMPONENT_DIRS` variable includes the paths of the newly added folders; otherwise, the compiler will not be able to find these components, leading to compilation failure.

In addition to modifying the project-level `CMakeLists.txt` file, the `CMakeLists.txt` file under the `main` folder also needs to be modified accordingly. The modified `main/CMakeLists.txt` file content is as follows:

```cmake
# Define source directories
set(src_dirs
    .
)

# Define include directories
set(include_dirs
    .
)

# No REQUIRES: ESP-IDF links `main` against all components in the project (see root CMakeLists.txt).
# New components under EXTRA_COMPONENT_DIRS do not need to be listed here.

# Register the component
idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
)

# Add compilation options
# component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)

```

!!! warning "Note"
    It is best not to explicitly write dependency relationships in the makefile under the main folder. If you do not write them, it will default to the search path of the entire project, which is the path included in the `EXTRA_COMPONENT_DIRS` variable we set above. This way, you do not need to explicitly write out the dependencies here, making the code structure clearer and more concise.

!!! note
    The core goal of this project is to create a sensor node, focusing on hardware design and basic driver implementation, without much involvement in middleware and application layer development. However, to make the project
    structure more complete and standardized, and to provide a foundation for future projects, we have still created these middleware and application layer folders.

## BRANCH DESCRIPTION

<!-- By applying the above modifications to both the AIOT-C-ZERO and AIOT-CPP-ZERO versions, we can obtain the AIOT-C-CORE and AIOT-CPP-CORE project frameworks, which will serve as the basis for core functionality development in subsequent projects. -->

For now, we will name this version AIoTNode-CORE-BLANK, as it has a preliminary framework but has not yet been filled with content.

## COMPONENTS AND MODULES

In this project, each specific functionality is implemented in the form of components, which can be modularly represented in the project code structure. Components are the basic code units in ESP-IDF projects, enhancing code reusability and maintainability. Each component contains its own source code files, header files, and CMake build files, allowing for independent compilation and linking to the main project.

!!! warning "Note"
    In applications, please pay special attention to the dependencies between components. If one component depends on another, this dependency must be correctly declared in the CMakeLists.txt file to ensure that the compiler can properly handle these dependencies and avoid compilation errors.