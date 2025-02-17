# PROJECT CONSTRUCTION

## PROJECT-LEVEL CMakelists.txt

```cmake
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "./driver" "./middleware" "./application")

project(DNESP32S3-CORE)

```

!!! note
    To be consistent with the project code architecture introduced previously, we recommend that you create a `driver`, `middleware`, and `application` directory in the project root directory and place the corresponding code in the corresponding directory. To incorporate these directories into the project, you need to add the path to these directories to the `EXTRA_COMPONENT_DIRS` variable in the project-level `CMakeLists.txt` file.

!!! danger
    Must note that `set(EXTRA_COMPONENT_DIRS "./driver" "./middleware" "./application")` should be placed before `project(DNESP32S3-CORE)`. Otherwise, the project will not be able to find the components in the directories. Refer to [LINK](https://blog.51cto.com/u_16099361/12129084).

