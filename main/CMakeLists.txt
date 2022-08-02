idf_component_register(SRCS "ble_hidd_demo_main.c"
                            "esp_hidd_prf_api.c"
                            "hid_dev.c"
                            "hid_device_le_prf.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_hid PRIV_REQUIRES esp32 esp_wifi esp_https_server esp_eth mdns lwip fatfs esp_https_ota esp_hid app_update)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
