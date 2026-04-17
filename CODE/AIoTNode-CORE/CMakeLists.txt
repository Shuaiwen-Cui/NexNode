# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# Disable IDF Component Manager: no registry downloads and no project-root dependencies.lock.
# Vendored components (e.g. driver/espressif__led_strip) use CMakeLists.txt only.
set(ENV{IDF_COMPONENT_MANAGER} 0)

# Extra components (e.g. driver/node_led): one subfolder with CMakeLists.txt = one component.
# With no REQUIRES in main/CMakeLists.txt, `main` depends on all built components automatically.
# `driver/espressif__led_strip` is a vendored offline copy — if you still have
# `managed_components/espressif__led_strip` from an old manifest, delete it to avoid duplicate components.
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/driver" "${CMAKE_SOURCE_DIR}/middleware" "${CMAKE_SOURCE_DIR}/application")
if(EXISTS "${CMAKE_SOURCE_DIR}/managed_components")
    list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/managed_components")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(AIoTNode)