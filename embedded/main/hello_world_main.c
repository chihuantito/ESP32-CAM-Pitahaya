#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_camera.h"
#include "driver/gpio.h"

// ==================#
// CONFIGURACIÓN     #
// ==================#
#define GPIO_FLASH_LED   4
#define MQTT_TOPIC_LED   "camara/led"

// #define WIFI_SSID       "Xiaomi_674E"
// #define WIFI_PASS       "12345678"

#define WIFI_SSID       "ZTE_2.4G_tUa575"
#define WIFI_PASS       "CuzCax11!"

#define MQTT_BROKER_URI "mqtt://192.168.1.2:1883"
#define MQTT_TOPIC      "camara/frame"

#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0      5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

static const char *TAG = "ESP32_CAM";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static EventGroupHandle_t s_status_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define MQTT_CONNECTED_BIT BIT1

// ======================#
// EVENTOS WI-FI Y MQTT  #
// ======================#
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_status_event_group, WIFI_CONNECTED_BIT | MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "Wi-Fi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_status_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Wi-Fi connected. Starting MQTT...");
        if (mqtt_client) {
            esp_mqtt_client_start(mqtt_client);
        }
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Broker Connected");
            xEventGroupSetBits(s_status_event_group, MQTT_CONNECTED_BIT);

            // Subscription to the LED control topic
            esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_LED, 0);
            ESP_LOGI(TAG, "Subscribed to: %s", MQTT_TOPIC_LED);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected");
            xEventGroupClearBits(s_status_event_group, MQTT_CONNECTED_BIT);
            break;

        case MQTT_EVENT_DATA:
            // Validate that the received message belongs to the LED topic
            if (event->topic_len == strlen(MQTT_TOPIC_LED) && strncmp(event->topic, MQTT_TOPIC_LED, event->topic_len) == 0) {
                if (event->data_len > 0) {
                    if (event->data[0] == '1') {
                       // gpio_set_level(GPIO_FLASH_LED, 1);
                        ESP_LOGI(TAG, "Command received: LED On");
                    } else if (event->data[0] == '0') {
                        gpio_set_level(GPIO_FLASH_LED, 0);
                        ESP_LOGI(TAG, "Command received: LED Off");
                    }
                }
            }
            break;

        default:
            break;
    }
}

// =========================#
// NETWORK INITIALIZATION   #
// =========================#
static void network_init(void) {
    s_status_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Disable power saving to ensure continuous transmission with no latency
    // ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));  // Disable power saving. The RF remains continuously on at 100%
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM)); // Turns off the radio between the beacon intervals sent by the router (approximately every 100 ms)
}

// ===========================#
// CAMERA INITIALIZATION      #
// ===========================#
static esp_err_t init_camera(void) {
    camera_config_t camera_config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_QVGA, // 320 x 240
        .jpeg_quality = 12,
        .fb_count = 2,
        .grab_mode = CAMERA_GRAB_LATEST
    };

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando cámara: 0x%x", err);
    }
    return err;
}

// =========================#
// TRANSMISSION TASK        #
// =========================#
void camera_task(void *pvParameters) {
    const EventBits_t bits_to_wait = WIFI_CONNECTED_BIT | MQTT_CONNECTED_BIT;

    while (1) {
        // Block until Wi-Fi and MQTT are ready
        EventBits_t uxBits = xEventGroupWaitBits(
            s_status_event_group, 
            bits_to_wait, 
            pdFALSE, 
            pdTRUE, 
            portMAX_DELAY
        );

        if ((uxBits & bits_to_wait) == bits_to_wait) {
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGE(TAG, "Failed to capture image");
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }

            int msg_id = esp_mqtt_client_publish(
                mqtt_client,
                MQTT_TOPIC,
                (const char *)fb->buf,
                fb->len,
                0, // QoS = 0
                0 // Retain = 0
            );

            if (msg_id < 0) {
                ESP_LOGE(TAG, "Error publishing to MQTT");
            } else {
                ESP_LOGI(TAG, "Image published to [%s]. Size: %zu bytes (~%zu KB)", MQTT_TOPIC, fb->len, fb->len / 1024);
            }

            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(200)); // ~5 FPS
        }
    }
}

// =======#
// MAIN   #
// =======#
void app_main(void) {
    // Initialize the LED pin
    gpio_reset_pin(GPIO_FLASH_LED);
    gpio_set_direction(GPIO_FLASH_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_FLASH_LED, 0);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    network_init();
    ESP_ERROR_CHECK(init_camera());

    // Optimized MQTT configuration (16 KB buffer)
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .buffer.out_size = 16384,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    // Task with appropriate priority and assigned to core 1
    xTaskCreatePinnedToCore(camera_task, "camera_task", 3072, NULL, 5, NULL, 1);
}