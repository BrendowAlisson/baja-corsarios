#include "../../include/esp_now_master.h"
#include "../../include/wifi_init.h"
#include "esp_crc.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <string.h>

#define TAG "esp_now_master"

#define ESPNOW_MAXDELAY 512

static xQueueHandle s_example_espnow_queue;

static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };

static void example_espnow_deinit(example_espnow_send_param_t *send_param);

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    example_espnow_event_t evt;
    example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    if (xQueueSend(s_example_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send send queue fail");
    }
}

/* Prepare ESPNOW data to be sent. */
void example_espnow_data_prepare(example_espnow_send_param_t *send_param)
{
    example_espnow_data_t *buf = (example_espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(example_espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
    buf->state = send_param->state;
    buf->seq_num = s_example_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;
    /* Fill all remaining bytes after the data with random values */
    esp_fill_random(buf->payload, send_param->len - sizeof(example_espnow_data_t));
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

static void set_peer_espnow(esp_now_peer_info_t *peer)
{
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);
}

static void set_parameters_espnow(example_espnow_send_param_t *parameters)
{
    memset(parameters, 0, sizeof(example_espnow_send_param_t));
    parameters->unicast = false;
    parameters->broadcast = true;
    parameters->state = 0;
    parameters->magic = esp_random();
    parameters->count = CONFIG_ESPNOW_SEND_COUNT;
    parameters->delay = CONFIG_ESPNOW_SEND_DELAY;
    parameters->len = CONFIG_ESPNOW_SEND_LEN;
    parameters->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);
    memcpy(parameters->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
}

static esp_err_t esp_now_initialize(example_espnow_send_param_t *send_param)
{
    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    if (s_example_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    return ESP_OK;
}

static void example_espnow_deinit(example_espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_example_espnow_queue);
    esp_now_deinit();
}

void send_message_espnow(example_espnow_send_param_t *send_param)
{
    if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
        ESP_LOGE(TAG, "Send error");
        example_espnow_deinit(send_param);
    }
}

void esp_now_master_init(void *p1)
{
    example_espnow_send_param_t *send_param = malloc(sizeof(example_espnow_send_param_t));
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    example_espnow_event_t evt;
    bool is_broadcast = false;

    wifi_init();
    esp_now_initialize(send_param);
    set_peer_espnow(peer);
    set_parameters_espnow(send_param);
    vTaskDelay(5000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Start sending broadcast data");
    send_message_espnow(send_param);

    while (xQueueReceive(s_example_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        switch (evt.id) {
            case EXAMPLE_ESPNOW_SEND_CB:
            {
                example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
                if (send_param->delay > 0) {
                    vTaskDelay(send_param->delay/portTICK_RATE_MS);
                }
                ESP_LOGI(TAG, "send data to "MACSTR"", MAC2STR(send_cb->mac_addr));
                memcpy(send_param->dest_mac, send_cb->mac_addr, ESP_NOW_ETH_ALEN);
                example_espnow_data_prepare(send_param);
                send_message_espnow(send_param);
                break;
            default:
                ESP_LOGE(TAG, "Callback type error: %d", evt.id);
                break;
            }
        }
    }
    vTaskDelay(portMAX_DELAY);
}
