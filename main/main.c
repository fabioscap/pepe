#include <stdio.h>
#include "freertos/FreeRTOS.h"

#include <nvs.h>
#include <nvs_flash.h>

#include <time.h> 

#include "feed_schedule.h"
#include "wifi_setup.h"
#include "my_http_server.h"
#include "drive_servo.h"
#include "esp_log.h"
#include <freertos/semphr.h>

// fetch ssid and password from flash memory.
void get_ssid_pwd(char** _ssid, char** _pwd);

void dummyTask(void* params);


void app_main(void) {
    // init flash memory.
    ESP_ERROR_CHECK(nvs_flash_init());

    // fetch wifi ssid and password.
    char* ssid;
    char* pwd;
    get_ssid_pwd(&ssid,&pwd);
    wifi_setup(ssid,pwd);

    //xTaskCreate(dummyTask,"dummyTask",2048,NULL,5,NULL);
    // start servo PWM library
    my_servo_init();

    // start feed schedule.
    schedule_handler = init_feed_schedule();
    
    // Now I can create an http server.
    httpd_handle_t server = start_webserver(schedule_handler);
    
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