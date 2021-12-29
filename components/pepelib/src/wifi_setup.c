#include <esp_netif.h>
#include <esp_wifi.h>
#include "freertos/event_groups.h"
#include <esp_sntp.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>

// ntp server.
#define NTP_SERVER_NAME "ntp1.inrim.it"

// FreeRTOS event group to signal when we are connected.
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT ( 1 << 0 ) // first bit.

// function to be called when an event is posted to the default loop.
void wifi_event_callback(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data){

    switch (id) {
        case WIFI_EVENT_STA_START:
            // try to connect to the network.
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ; // the event task starts DHCP.
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            // handle problems...
            ESP_LOGE("wifi","disconnected!");
            // ...
            break;
        case IP_EVENT_STA_GOT_IP:
            // I can now set the connected bit up
            // so that the program can continue.
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        default:
            ;
            ESP_LOGE("wifi","unexpected event!");
            break;
    }
}

void wifi_setup (char* ssid, char* pwd) {
    // init transport layer.
    ESP_ERROR_CHECK(esp_netif_init());

    // create the default event loop, a task
    // that handles the wifi events.
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // initialize the wifi event group (a semaphore).
    s_wifi_event_group = xEventGroupCreate();

    // register my event handler to the default
    // event loop, listening to wifi and ip events.
    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event_callback,NULL);
    esp_event_handler_register(IP_EVENT,  ESP_EVENT_ANY_ID,wifi_event_callback,NULL);
    

    //  ?
    esp_netif_create_default_wifi_sta();

    // init wifi with default configuration.
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    // set wifi mode to station.
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // set wifi configs (in my case just ssid and password).
    wifi_config_t cfg = {};
    strcpy((char*)cfg.sta.ssid,ssid);
    strcpy((char*)cfg.sta.password,pwd);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&cfg));

    // start the driver. Will send the event WIFI_EVENT_STA_START
    // which I will handle by connecting to my access point
    ESP_ERROR_CHECK(esp_wifi_start());

    // stall the program until wifi connection is OK
    xEventGroupWaitBits(s_wifi_event_group,
                        WIFI_CONNECTED_BIT, //wait for the connected bit.
                        pdFALSE, // do not clear the bit.
                        pdFALSE, // wait for all bits: in my case it does not matter.
                        portMAX_DELAY // wait forever.
                        );

    // at this point wifi connection is OK.
    
    // setup clock sync.
    setenv("TZ","GMT-1", 1); tzset();    // set time zone (sign is inverted in POSIX standard).

    sntp_setoperatingmode(SNTP_OPMODE_POLL); // poll every hour.
    sntp_setservername(0, NTP_SERVER_NAME);
    sntp_init();

    // wait for sntp to sync.
    sntp_sync_status_t st = sntp_get_sync_status();;
    while(st != SNTP_SYNC_STATUS_COMPLETED) {
        vTaskDelay(250 / portTICK_RATE_MS);
        st = sntp_get_sync_status();
    }
}