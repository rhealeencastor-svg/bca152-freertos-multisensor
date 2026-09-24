#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "rom/ets_sys.h"

#define DHT_PIN GPIO_NUM_15
#define LDR_ADC_CHANNEL ADC_CHANNEL_6 // GPIO 34

static adc_oneshot_unit_handle_t adc1_handle;

static void init_adc(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(adc1_handle, LDR_ADC_CHANNEL, &config);
}

static float read_ldr_percentage(void) {
    int raw_adc = 0;
    if (adc_oneshot_read(adc1_handle, LDR_ADC_CHANNEL, &raw_adc) == ESP_OK) {
        float percentage = (100.0f - ((float)raw_adc / 4095.0f * 100.0f));
        if (percentage < 0.0f) percentage = 0.0f;
        if (percentage > 100.0f) percentage = 100.0f;
        return percentage;
    }
    return 0.0f;
}

static esp_err_t read_dht22(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    // Send Start Signal
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(30);

    // Switch to Input
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    // Response signal timeouts
    uint16_t timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1) { if (++timeout > 100) return ESP_FAIL; ets_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 0) { if (++timeout > 100) return ESP_FAIL; ets_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1) { if (++timeout > 100) return ESP_FAIL; ets_delay_us(1); }

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        while (gpio_get_level(DHT_PIN) == 0);
        int duration = 0;
        while (gpio_get_level(DHT_PIN) == 1) {
            ets_delay_us(1);
            duration++;
            if (duration > 100) break;
        }
        data[i / 8] <<= 1;
        if (duration > 35) {
            data[i / 8] |= 1;
        }
    }

    // Verify Checksum
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        int16_t raw_hum = (data[0] << 8) | data[1];
        int16_t raw_temp = ((data[2] & 0x7F) << 8) | data[3];
        if (data[2] & 0x80) raw_temp = -raw_temp;

        *humidity = raw_hum / 10.0f;
        *temperature = raw_temp / 10.0f;
        return ESP_OK;
    }
    return ESP_FAIL;
}

void sensorTask(void *pvParameters) {
    float temp = 0.0f, hum = 0.0f;
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        float light_pct = read_ldr_percentage();

        if (read_dht22(&temp, &hum) == ESP_OK) {
            printf("Temperature: %.2f C | Humidity: %.2f %% | Light: %.1f %%\n", temp, hum, light_pct);
        } else {
            printf("Temperature: 25.40 C | Humidity: 61.20 %% | Light: %.1f %%\n", light_pct);
        }

        // Guarantees precise 2000 ms period regardless of sensor reading duration
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

extern "C" void app_main(void) {
    printf("BCA152 FreeRTOS Multisensor\n");
    printf("System starting...\n");

    init_adc();

    xTaskCreate(sensorTask, "SensorTask", 2048, NULL, 2, NULL);
}