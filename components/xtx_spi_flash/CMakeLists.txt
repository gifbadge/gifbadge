idf_component_register(SRCS "spi_flash_chip_drivers.c" "spi_flash_chip_xtx.c" INCLUDE_DIRS "include"
        REQUIRES hal
        PRIV_REQUIRES spi_flash
        LDFRAGMENTS linker.lf)
idf_component_add_link_dependency(FROM spi_flash)