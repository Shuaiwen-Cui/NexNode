# CORE FUNCTION DEVELOPMENT

!!! info "Introduction"
    In the initialization chapter, we have introduced how to set up the development environment and the basic project structure. This chapter will build upon that foundation to expand the project code and create a code framework for an IoT node with core functionalities.

## PROJECT FRAMEWORK MODIFICATION

The previous version was just the most basic "Hello World" example code, which could not meet the needs of practical applications. Hierarchical and modular management of code is the foundation for large-scale project development. At the project level, we added `driver`, `middleware`, and `application` folders at the same level as the `main` folder to store hardware driver code, middleware code, and application layer code, respectively. Each of these newly added folders contains a readme.txt file to avoid issues with empty folders not being managed by Git. The structure is shown in the figure below:

![](PROJ-ARCH.png){width="250"}

After completing the above operations, we also need to modify the `CMakeLists.txt` file to ensure that the newly added folders can be correctly compiled. The modified project-level `CMakeLists.txt` file content is as follows:

```cmake
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "./driver" "./middleware" "./application")

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

# Define required components
set(requires
)

# Register the component
idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES ${requires}
)

# Add compilation options
# component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)
```

!!! tip "Suggestion"
    The core goal of this project is to create a sensor node, focusing on hardware design and basic driver implementation, without much involvement in middleware and application layer development. However, to make the project
    structure more complete and standardized, and to provide a foundation for future projects, we have still created these middleware and application layer folders.

## VERSION DESCRIPTION

By applying the above modifications to both the AIOT-C-ZERO and AIOT-CPP-ZERO versions, we can obtain the AIOT-C-CORE and AIOT-CPP-CORE project frameworks, which will serve as the basis for core functionality development in subsequent projects.