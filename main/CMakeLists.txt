function(get_component_library component_name output_var)
    idf_build_get_property(build_components BUILD_COMPONENTS)
    set(TARGET_COMPONENT "")
    foreach(COMPONENT ${build_components})
        if(COMPONENT MATCHES "${component_name}" OR COMPONENT MATCHES "espressif__${component_name}")
            set(TARGET_COMPONENT ${COMPONENT})
            break()
        endif()
    endforeach()
    if(TARGET_COMPONENT STREQUAL "")
        message(FATAL_ERROR "Component '${component_name}' not found.")
    else()
        idf_component_get_property(COMPONENT_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
        set(${output_var} ${COMPONENT_LIB} PARENT_SCOPE)
    endif()
endfunction()

idf_component_register(
    SRCS
        "main.cpp"
        "display.cpp"
        "wifi_connect.cpp"
        "mqtt_hrv.cpp"
        "hrv_ui.cpp"
        "hrv_status_store.cpp"
        "hrv_power.cpp"
        "hrv_imu_wake.c"
        "hrv_ota.cpp"
        "hrv_battery.c"
        "hrv_user_btn.cpp"
        "board_display.c"
    INCLUDE_DIRS "include"
    REQUIRES
        nvs_flash
        esp_wifi
        esp_netif
        esp_event
        esp_timer
        mqtt
        json
        esp_board_manager
        esp_lvgl_port
        esp_driver_ledc
        esp-wifi-connect
        bmi270_sensor
        i2c_bus
        driver
        app_update
        esp_https_ota
        esp_http_client
        mbedtls
        esp_adc
        button
)

get_component_library("lvgl" LVGL_LIB)
target_compile_options(${LVGL_LIB} PUBLIC "-Wno-attributes")
