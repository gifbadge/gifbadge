set(GRAPHVIZ_IGNORE_TARGETS "^libgcc_cxx$|^gcc$|^stdcpp_deps$|^stdc.*|^c$|^m$|\.a$|^\-.*|^__idf_cxx|^__idf_newlib|^__idf_xtensa|^__idf_esp_common|^__idf_driver$|^__idf_esp_driver_gpio|^__idf_esp_hw_support|^__idf_hal|^__idf_esp_system|^__idf_log|^__idf_heap|^__idf_soc|^__idf_esp_rom|^__idf_espressif__cmake_utilities|^__idf_freertos|^__idf_pthread|^__idf_build_target|^__idf_esp_mm|^__idf_esp_pm")
set(GRAPHVIZ_UNKNOWN_LIBS FALSE)
set(GRAPHVIZ_GENERATE_PER_TARGET FALSE)
set(GRAPHVIZ_GENERATE_DEPENDERS FALSE)
# cleanup .dot regex "node\d+" -> "node\d+" \[ style = dotted ] // Gifbadge.elf -> (?>(?=__idf_main|__idf_gifbadge_hal_esp32)|.*)$