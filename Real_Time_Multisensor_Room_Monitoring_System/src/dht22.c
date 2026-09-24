#include "dht22.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t dht22_read(int gpio_num, float *temperature, float *humidity)
{
    uint8_t data[5] = {0};

    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_num, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(gpio_num, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);

    int timeout = 0;
    while (gpio_get_level(gpio_num) == 1) { if (++timeout > 100) return ESP_FAIL; esp_rom_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(gpio_num) == 0) { if (++timeout > 100) return ESP_FAIL; esp_rom_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(gpio_num) == 1) { if (++timeout > 100) return ESP_FAIL; esp_rom_delay_us(1); }

    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level(gpio_num) == 0) { if (++timeout > 100) return ESP_FAIL; esp_rom_delay_us(1); }
        esp_rom_delay_us(35);
        int bit = gpio_get_level(gpio_num);
        timeout = 0;
        while (gpio_get_level(gpio_num) == 1) { if (++timeout > 100) return ESP_FAIL; esp_rom_delay_us(1); }
        data[i / 8] <<= 1;
        if (bit) data[i / 8] |= 1;
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) return ESP_ERR_INVALID_CRC;

    *humidity = ((data[0] << 8) | data[1]) / 10.0f;
    int16_t temp_raw = ((data[2] & 0x7F) << 8) | data[3];
    *temperature = temp_raw / 10.0f;
    if (data[2] & 0x80) *temperature = -*temperature;

    return ESP_OK;
}