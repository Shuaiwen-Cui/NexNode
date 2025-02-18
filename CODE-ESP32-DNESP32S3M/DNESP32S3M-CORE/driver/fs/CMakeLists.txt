set(src_dirs
    .
)

set(include_dirs
    include
)

set(requires
    driver
    spiffs
    fatfs
    vfs
    spi
    spi_sdcard
    sdmmc
    led
    lcd
)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})