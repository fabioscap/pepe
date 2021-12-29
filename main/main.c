#include <stdio.h>


#include <nvs.h>
#include <nvs_flash.h>

#include <time.h> 


#include "feed_schedule.h"
#include "wifi_setup.h"
#include "my_http_server.h"

schedule_list_handler_t init_feed_schedule();
void update_feed_schedule(queue_element_t* fs,uint8_t n);
void print_feed_schedule(queue_element_t* fs, uint8_t n);

// fetch ssid and password from flash memory.
void get_ssid_pwd(char** _ssid, char** _pwd);

void dummyTask(void* params);
/*
esp_err_t schedule_handler(httpd_req_t *req) {
    int p = 0;
    char buff[6*MAX_FEED_IN_A_DAY];
    // attempt to take semaphore 
    if (xSemaphoreTake(schedule_semaphore, (TickType_t) 10) == pdTRUE) {
        for (int i=0; i< schedule_list.size; ++i) {
            sprintf(buff+p,"%02u:%02u,",schedule_list.list[i].hour,schedule_list.list[i].minute);
            p += 6;
        }
        buff[--p] = '\0';
        xSemaphoreGive(schedule_semaphore);
        httpd_resp_send(req, buff, HTTPD_RESP_USE_STRLEN);
        // opt: wake up feed task.
    }
    else {
        // failure to get semaphore.
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
*/
void app_main(void) {
    // init flash memory.
    ESP_ERROR_CHECK(nvs_flash_init());

    // fetch wifi ssid and password.
    char* ssid;
    char* pwd;
    get_ssid_pwd(&ssid,&pwd);
    wifi_setup(ssid,pwd);

    //xTaskCreate(dummyTask,"dummyTask",2048,NULL,5,NULL);

    // start feed schedule.
    schedule_list_handler_t schedule_handler = init_feed_schedule();
    
    // Now I can create an http server.
    httpd_handle_t server = start_webserver(schedule_handler);

    /*
    httpd_register_uri_handler(server, &get_index);
    httpd_register_uri_handler(server, &get_schedule);
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
    */
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

void dummyTask(void* params) {
    time_t t_now; struct tm now_human; // time in human format.
    time(&t_now); localtime_r(&t_now,&now_human);
    while(1) {
        printf("%02u:%02u\n",now_human.tm_hour,now_human.tm_min);
        vTaskDelay(1000 / portTICK_RATE_MS);
        time(&t_now); localtime_r(&t_now,&now_human);
    }
}