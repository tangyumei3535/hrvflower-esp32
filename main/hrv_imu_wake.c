/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hrv_imu_wake.h"

#include "sdkconfig.h"

#if CONFIG_HRV_IMU_WAKE_ENABLE

#include "bmi270_api.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"

static const char *TAG = "hrv_imu";

static bmi270_handle_t s_bmi_handle;
static i2c_bus_handle_t s_i2c_bus;
static bool s_ready;

#define IMU_INT_GPIO  CONFIG_HRV_IMU_INT_GPIO
#define I2C_SDA_GPIO  CONFIG_HRV_IMU_I2C_SDA_GPIO
#define I2C_SCL_GPIO  CONFIG_HRV_IMU_I2C_SCL_GPIO

static esp_err_t bmi270_bus_init(void)
{
    if (s_bmi_handle != NULL) {
        return ESP_OK;
    }

    const i2c_config_t bus_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SCL_GPIO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    s_i2c_bus = i2c_bus_create(I2C_NUM_0, &bus_cfg);
    if (s_i2c_bus == NULL) {
        return ESP_FAIL;
    }

    if (bmi270_sensor_create(s_i2c_bus, &s_bmi_handle, bmi270_toy_config_file, 0) != ESP_OK
            || s_bmi_handle == NULL) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

static int8_t bmi270_set_any_motion(bmi270_handle_t dev)
{
    struct bmi2_sens_config config[2] = {
        {.type = BMI2_ACCEL},
        {.type = BMI2_GYRO},
    };
    struct bmi2_int_pin_config pin = {0};
    uint8_t sens[] = {BMI2_ACCEL, BMI2_GYRO};
    int8_t rslt;

    rslt = bmi2_get_sensor_config(config, 2, dev);
    if (rslt != BMI2_OK) {
        return rslt;
    }

    config[BMI2_ACCEL].cfg.acc.odr = BMI2_ACC_ODR_200HZ;
    config[BMI2_ACCEL].cfg.acc.range = BMI2_ACC_RANGE_16G;
    config[BMI2_ACCEL].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;
    config[BMI2_ACCEL].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;

    config[BMI2_GYRO].cfg.gyr.odr = BMI2_GYR_ODR_200HZ;
    config[BMI2_GYRO].cfg.gyr.range = BMI2_GYR_RANGE_2000;
    config[BMI2_GYRO].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;
    config[BMI2_GYRO].cfg.gyr.noise_perf = BMI2_PERF_OPT_MODE;
    config[BMI2_GYRO].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

    rslt = bmi2_set_sensor_config(config, 2, dev);
    if (rslt != BMI2_OK) {
        return rslt;
    }

    rslt = bmi2_set_adv_power_save(BMI2_DISABLE, dev);
    if (rslt != BMI2_OK) {
        return rslt;
    }

    rslt = bmi2_sensor_enable(sens, 2, dev);
    if (rslt != BMI2_OK) {
        return rslt;
    }

    const uint8_t map = BMI270_TOY_INT_ANY_MOT_MASK;
    rslt = bmi2_set_regs(BMI2_INT1_MAP_FEAT_ADDR, &map, 1, dev);
    if (rslt != BMI2_OK) {
        return rslt;
    }

    pin.pin_type = BMI2_INT1;
    pin.pin_cfg[0].input_en = BMI2_INT_INPUT_DISABLE;
    pin.pin_cfg[0].lvl = BMI2_INT_ACTIVE_HIGH;
    pin.pin_cfg[0].od = BMI2_INT_PUSH_PULL;
    pin.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;
    pin.int_latch = BMI2_INT_LATCH;
    rslt = bmi2_set_int_pin_config(&pin, dev);
    if (rslt != BMI2_OK) {
        return rslt;
    }

    return bmi270_enable_toy_any_motion(dev, BMI2_ENABLE);
}

void hrv_imu_wake_on_boot(void)
{
    gpio_hold_dis(IMU_INT_GPIO);

    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
        return;
    }
    if ((esp_sleep_get_ext1_wakeup_status() & (1ULL << IMU_INT_GPIO)) == 0) {
        return;
    }

    ESP_LOGI(TAG, "Wake: IMU GPIO%d", IMU_INT_GPIO);
    if (s_bmi_handle != NULL) {
        uint8_t st = 0;
        bmi2_get_regs(BMI2_INT_STATUS_0_ADDR, &st, 1, s_bmi_handle);
    }
}

esp_err_t hrv_imu_wake_prepare(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    if (bmi270_bus_init() != ESP_OK) {
        ESP_LOGE(TAG, "BMI270 init failed");
        return ESP_FAIL;
    }

    if (bmi270_set_any_motion(s_bmi_handle) != BMI2_OK) {
        ESP_LOGE(TAG, "any-motion config failed");
        return ESP_FAIL;
    }

    s_ready = true;
    ESP_LOGI(TAG, "BMI270 any-motion ready (INT GPIO%d)", IMU_INT_GPIO);
    return ESP_OK;
}

esp_err_t hrv_imu_wake_arm_for_deep_sleep(void)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t st = 0;
    bmi2_get_regs(BMI2_INT_STATUS_0_ADDR, &st, 1, s_bmi_handle);
    vTaskDelay(pdMS_TO_TICKS(20));

    const gpio_config_t io = {
        .pin_bit_mask = (1ULL << IMU_INT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&io) != ESP_OK) {
        return ESP_FAIL;
    }
    gpio_hold_en(IMU_INT_GPIO);

    return esp_sleep_enable_ext1_wakeup((1ULL << IMU_INT_GPIO), ESP_EXT1_WAKEUP_ANY_HIGH);
}

#else

void hrv_imu_wake_on_boot(void) {}

esp_err_t hrv_imu_wake_prepare(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t hrv_imu_wake_arm_for_deep_sleep(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

#endif
