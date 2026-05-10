/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "waveshare_rgb_lcd_port.h"
#include "ui.h"
#include "driver/gpio.h"

#include "driver/i2c.h"
#include <stdio.h>

#define I2C_MASTER_SDA_IO    15   
#define I2C_MASTER_SCL_IO    16   
#define I2C_MASTER_FREQ_HZ   400000  // 400kHz I2C clock
#define I2C_MASTER_PORT      I2C_NUM_1  
#define TCA9534_ADDR         0x18  // device address

// I2C init
void i2c_master_init() {
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };
    i2c_param_config(I2C_MASTER_PORT, &config);
    i2c_driver_install(I2C_MASTER_PORT, config.mode, 0, 0, 0);
}

// Write to the TCA9534 register
esp_err_t i2c_write_register(uint8_t reg_addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TCA9534_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

//Read the TCA9534 register
esp_err_t i2c_read_register(uint8_t reg_addr, uint8_t *data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TCA9534_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TCA9534_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
esp_err_t i2c_write_byte(uint8_t device_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}


void app_main()
{
    i2c_master_init();
    vTaskDelay(pdMS_TO_TICKS(50)); 
    
    uint8_t config_val = 0xFF; // All are input by default
    i2c_read_register(0x03, &config_val);
    config_val &= ~(1 << 1); // P1 Set as output
    config_val &= ~(1 << 2); // P2 Set as output
    config_val &= ~(1 << 3); // P3 Set as output
    config_val &= ~(1 << 4); // P4 Set as output
    i2c_write_register(0x03, config_val);
    
    // Set P3 output low and P4 output low
    uint8_t output_val = 0x00; // Default output low
    i2c_read_register(0x01, &output_val);
    output_val |= (1 << 1); // P1 = 1

        // Configure pin 1 for output mode
        gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT);
        // Pull pin 1 down
        gpio_set_level(GPIO_NUM_1, 0);

    output_val &= ~(1 << 2); // P2 = 0
    vTaskDelay(pdMS_TO_TICKS(20));
    output_val |= (1 << 2); // P2 = 1

    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_direction(GPIO_NUM_1, GPIO_MODE_INPUT);

    i2c_write_register(0x01, output_val);

    gpio_reset_pin(19);
    gpio_set_direction(19, GPIO_MODE_OUTPUT);

    waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB LCD 
    
    ESP_LOGI(TAG, "Display LVGL demos");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1)) {
        ui_init();
        lvgl_port_unlock();
    }
}
