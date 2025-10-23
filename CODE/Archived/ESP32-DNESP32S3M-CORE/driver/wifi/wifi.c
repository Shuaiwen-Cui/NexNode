// #include "wifi.h"

// #ifdef __cplusplus
// extern "C" {
// #endif

// /* Event group for WiFi status */
// static EventGroupHandle_t   wifi_event;
// static const char *TAG = "static_ip";
// char lcd_buff[100] = {0};

// /**
//  * @brief       Connection display helper
//  * @param       flag: 2 -> connected; 1 -> connection failed; 0 -> connecting
//  * @retval      none
//  */
// void connet_display(uint8_t flag)
// {
//     if(flag == 2)
//     {
//         lcd_fill(0,40,lcd_self.width,lcd_self.height,WHITE);
//         sprintf(lcd_buff, "SSID:%s",DEFAULT_SSID);
//         lcd_show_string(0, 40, lcd_self.width, 16, 16, lcd_buff, BLUE);
//         sprintf(lcd_buff, "PSW:%s",DEFAULT_PWD);
//         lcd_show_string(0, 60, lcd_self.width, 16, 16, lcd_buff, BLUE);
//     }
//     else if (flag == 1)
//     {
//         lcd_show_string(0, 40, lcd_self.width, 16, 16, "wifi connecting fail", BLUE);
//     }
//     else
//     {
//         lcd_show_string(0, 40, lcd_self.width, 16, 16, "wifi connecting......", BLUE);
//     }
// }

// /**
//  * @brief       WiFi event handler callback
//  * @param       arg: pointer passed to handler (unused)
//  * @param       event_base: event base (WIFI_EVENT or IP_EVENT)
//  * @param       event_id: event identifier
//  * @param       event_data: event-specific data
//  * @retval      none
//  */
// static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
// {
//     static int s_retry_num = 0;

//     /* WiFi station start event */
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
//     {
//         connet_display(0);
//         esp_wifi_connect();
//     }
//     /* WiFi connected event */
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
//     {
//         connet_display(2);
//     }
//     /* WiFi disconnected / connection failed event */
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
//     {
//         /* attempt reconnect */
//         if (s_retry_num < 20)
//         {
//             esp_wifi_connect();
//             s_retry_num++;
//             ESP_LOGI(TAG, "retry to connect to the AP");
//         }
//         else
//         {
//             xEventGroupSetBits(wifi_event, WIFI_FAIL_BIT);
//         }

//         ESP_LOGI(TAG,"connect to the AP fail");
//     }
//     /* Station obtained IP from AP */
//     else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
//     {
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
//         s_retry_num = 0;
//         xEventGroupSetBits(wifi_event, WIFI_CONNECTED_BIT);
//     }
// }

// /**
//  * @brief       Initialize WiFi in station mode
//  * @param       none
//  * @retval      none
//  */
// void wifi_sta_init(void)
// {
//     static esp_netif_t *sta_netif = NULL;
//     wifi_event= xEventGroupCreate();    /* Create an event group */
//     /* Initialize network interface (TCP/IP stack) */
//     ESP_ERROR_CHECK(esp_netif_init());
//     /* Create default event loop */
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     sta_netif= esp_netif_create_default_wifi_sta();
//     assert(sta_netif);
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );
//     ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL) );
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));    
//     wifi_config_t  wifi_config = WIFICONFIG();
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
//     ESP_ERROR_CHECK(esp_wifi_start());

//     /* Wait for connect or fail event and for IP to be obtained */
//     EventBits_t bits = xEventGroupWaitBits( wifi_event,
//                                             WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//                                             pdFALSE,
//                                             pdFALSE,
//                                             portMAX_DELAY);

//     /* Check connection result */
//     if (bits & WIFI_CONNECTED_BIT)
//     {
//         ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
//                  DEFAULT_SSID, DEFAULT_PWD);
//     }
//     else if (bits & WIFI_FAIL_BIT)
//     {
//         connet_display(1);
//         ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
//                  DEFAULT_SSID, DEFAULT_PWD);
//     }
//     else
//     {
//         ESP_LOGE(TAG, "UNEXPECTED EVENT");
//     }

//     vEventGroupDelete(wifi_event);
// }

// #ifdef __cplusplus
// }
// #endif