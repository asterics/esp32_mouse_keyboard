idf_component_register(SRCS "./ble_hidd_demo_main.c" "./keyboard.c" "ble_hid/hid_device_le_prf.c" "ble_hid/hal_ble.c" "ble_hid/hid_dev.c"
                       INCLUDE_DIRS "." "ble_hid"                       # Edit following two lines to set component requirements (see docs)
                       REQUIRES 
                       PRIV_REQUIRES )

