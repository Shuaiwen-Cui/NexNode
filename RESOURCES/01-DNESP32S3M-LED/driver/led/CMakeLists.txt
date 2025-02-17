# idf_component_register(SRCS "led.c"
#                     INCLUDE_DIRS "include"
#                     REQUIRES driver)

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