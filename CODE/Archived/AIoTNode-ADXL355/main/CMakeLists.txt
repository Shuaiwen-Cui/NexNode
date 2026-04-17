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
    PRIV_REQUIRES node_acc_adxl355 node_spi nvs_flash
)

# Add compilation options
# component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)
