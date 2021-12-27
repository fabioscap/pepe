#include <stdio.h>
#include <string.h>

#include <nvs.h>
#include <nvs_flash.h>

#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_sntp.h>

#include <esp_event.h>
#include <esp_err.h>
#include <esp_log.h>

#include "freertos/event_groups.h"

// ntp server.
#define NTP_SERVER_NAME "ntp1.inrim.it"

// handle to the queue
static QueueHandle_t feed_queue;
// maximun schedules in a day
#define MAX_FEED_IN_A_DAY 8

typedef struct queue_element {
    uint8_t used;
    uint8_t hour:7;
    uint8_t minute;
    time_t activation_time;
} queue_element_t;
typedef struct schedule_list {
    queue_element_t list[MAX_FEED_IN_A_DAY];
    uint8_t size;
} schedule_list_t;

void init_feed_schedule();
void update_feed_schedule(queue_element_t* fs,uint8_t n);
void print_feed_schedule(queue_element_t* fs, uint8_t n);

// FreeRTOS event group to signal when we are connected.
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT ( 1 << 0 ) // first bit.

// function to be recalled when an event is posted to the default loop.
void wifi_event_callback(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data);

// fetch ssid and password from flash memory.
void get_ssid_pwd(char** _ssid, char** _pwd);

// start the webserver.
httpd_handle_t start_webserver(void);

esp_err_t get_handler(httpd_req_t *req);
esp_err_t schedule_handler(httpd_req_t *req);

void dummyTask(void* params) {
    time_t t_now; struct tm now_human; // time in human format.
    time(&t_now); localtime_r(&t_now,&now_human);
    while(1) {
        printf("%02u:%02u\n",now_human.tm_hour,now_human.tm_min);
        vTaskDelay(1000 / portTICK_RATE_MS);
        time(&t_now); localtime_r(&t_now,&now_human);
    }
}

void app_main(void) {
    
    // init flash memory.
    ESP_ERROR_CHECK(nvs_flash_init());

    // fetch wifi ssid and password.
    char* ssid;
    char* pwd;
    get_ssid_pwd(&ssid,&pwd);

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

    // wait for sntp to sync
    
    xTaskCreate(dummyTask,"dummyTask",2048,NULL,5,NULL);

    // create the queue that will hold the schedule.
    feed_queue = xQueueCreate(MAX_FEED_IN_A_DAY, // queue length
                              sizeof(queue_element_t));

    // wait for sntp to sync.
    sntp_sync_status_t st = sntp_get_sync_status();;
    while(st != SNTP_SYNC_STATUS_COMPLETED) {
        vTaskDelay(250 / portTICK_RATE_MS);
        st = sntp_get_sync_status();
    }

    // start feed schedule.
    init_feed_schedule();

    // Now I can create an http server.
    httpd_handle_t server = start_webserver();
    

}


void get_ssid_pwd(char** _ssid, char** _pwd) {
    
    size_t ssid_l,pwd_l;
   
    nvs_handle_t handle;

    ESP_ERROR_CHECK(nvs_open("storage", NVS_READONLY, &handle));
    ESP_ERROR_CHECK(nvs_get_str(handle, "SSID", NULL, &ssid_l));
    ESP_ERROR_CHECK(nvs_get_str(handle, "pwd", NULL, &pwd_l));

    *_ssid = (char*)malloc(ssid_l*sizeof(char));
    *_pwd = (char*)malloc(pwd_l*sizeof(char));
    assert(*_ssid);
    assert(*_pwd);
    
    ESP_ERROR_CHECK(nvs_get_str(handle, "SSID", *_ssid, &ssid_l));
    ESP_ERROR_CHECK(nvs_get_str(handle, "pwd", *_pwd, &pwd_l));

    nvs_close(handle);
}

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

httpd_handle_t start_webserver(void) {

    // default http server configuration.
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // server handle.
    httpd_handle_t server = NULL;

    httpd_uri_t get_index = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL,
    };

    httpd_uri_t get_schedule = {
        .uri = "/schedule",
        .method = HTTP_GET,
        .handler = schedule_handler,
        .user_ctx = NULL,
    };

    // start server.
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_register_uri_handler(server, &get_index);
    httpd_register_uri_handler(server, &get_schedule);
    return server;
}

esp_err_t schedule_handler(httpd_req_t *req) {
    char buff[6*MAX_FEED_IN_A_DAY];
    for (int i=0; i<fs_dim; ++i) {

    }
    return ESP_OK;
}

esp_err_t get_handler(httpd_req_t *req)
{   
    // fetch html page
    extern const uint8_t index_html[] asm("index_html");
    httpd_resp_send(req, (char*)index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


void schedule_task(void* params) {
    time_t now;
    struct tm now_human; // time in human format.

    while(1) {
        // get current time.
        time(&now);
        localtime_r(&now,&now_human);
    }
}

void init_feed_schedule() {
    #ifdef FETCH_SCHEDULE_FROM_NVS
    ;
    #endif
    #ifndef FETCH_SCHEDULE_FROM_NVS
    queue_element_t s0 = {.hour=5,.minute=30};
    queue_element_t s1 = {.hour=22,.minute=40};
    fs = (queue_element_t*)malloc(2*sizeof(queue_element_t));
    fs[0] = s0; fs[1] = s1; fs_dim = 2;
    #endif
    update_feed_schedule(fs,fs_dim);


}

// sorts by the closest in time
int queue_element_comp(const void* a, const void* b) {
    queue_element_t* _a = (queue_element_t*)a;
    queue_element_t* _b = (queue_element_t*)b;
    
    time_t t_now; 
    time(&t_now); 
    /* sort by hour and minute
    struct tm now_human; // time in human format.
    localtime_r(&t_now,&now_human);
    queue_element_t now = {.hour=now_human.tm_hour,.minute=now_human.tm_min};
    if (_a->hour - now.hour > _b->hour - now.hour) return -1;
    if (_a->hour - now.hour < _b->hour - now.hour) return 1;
    if (_a->minute - now.minute > _b->minute - now.minute) return -1;
    if (_a->minute - now.minute < _b->minute - now.minute) return 1;
    return 0;
    */
    // sort by activation time
    if (_a->activation_time > _b->activation_time) return -1;
    if (_a->activation_time < _b->activation_time) return 1;
    return 0;
}

void print_feed_schedule(queue_element_t* fs, uint8_t n) {
    char buff[n*6 + 2]; //HH:MM -> 6 chars
    int p = 0;
    buff[p++] = '[';
    for (int i=0; i<n; ++i) {
        sprintf(buff+p,"%02u:%02u ",fs[i].hour,fs[i].minute);
        p += 6;
    }
    buff[--p] = ']';
    buff[++p] = '\n';
    buff[++p] = 0;
    printf(buff);

}

void update_feed_schedule(queue_element_t* fs,uint8_t n) {

    // clear previous feed_queue
    xQueueReset(feed_queue);

    // set activation times for each timestamp.
    time_t t_now; time(&t_now); // fetch current time.
    struct tm now_human; localtime_r(&t_now,&now_human); // time in human format.

    // get current clock time in seconds
    time_t now_timestamp_in_seconds = now_human.tm_sec +
                                      now_human.tm_min * 60 + 
                                      now_human.tm_hour * 60 * 60;
    time_t this_day = t_now - now_timestamp_in_seconds;
    time_t day_offset = 24*60*60; // a day offset in seconds.

    for (int i=0; i<n; ++i){
        // two cases:
        // 1- timestamp is greater than current time -> set it this day.
        // 2- timestamp is lower than current time -> set it the next day.
        queue_element_t timestamp = fs[i];
        time_t feed_timestamp_in_seconds = timestamp.hour*60*60 + timestamp.minute*60;

        if (feed_timestamp_in_seconds > now_timestamp_in_seconds) { 
            //case 1.
            timestamp.activation_time = this_day+feed_timestamp_in_seconds;
        }
        else {
            // case 2.
            timestamp.activation_time = this_day+day_offset+feed_timestamp_in_seconds;
        }
    }

    // sort the timestamps.
    qsort(fs,n,sizeof(queue_element_t),queue_element_comp);

    // send ordered schedule to queue.
    for (int i=0; i<n; ++i) 
        xQueueSendToBack(feed_queue,fs+i,portMAX_DELAY);
    print_feed_schedule(fs,n);
}
