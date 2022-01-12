#include <stdio.h>
#include "freertos/FreeRTOS.h"

#include <nvs.h>
#include <nvs_flash.h>

#include <time.h>
#include <math.h> 

#include "feed_schedule.h"
#include "wifi_setup.h"
#include "my_http_server.h"
#include "drive_servo.h"

#include <esp_err.h>

static servo_handle_t my_servo;
static schedule_list_handle_t my_schedule;

// fetch ssid and password from flash memory.
void get_ssid_pwd(char** _ssid, char** _pwd);

void schedule_callback(void* params);

esp_err_t web_pwm_handler(httpd_req_t *req);
esp_err_t send_schedule(httpd_req_t *req);

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
    // start my servo PWM library
    my_servo = my_servo_init();

    // start feed schedule.
    my_schedule = init_feed_schedule(schedule_callback);
    
    // Now I can create an http server.
    httpd_handle_t server = start_webserver(my_schedule);
    // register all handlers
    httpd_uri_t turn = {
        .uri = "/pwm",
        .method = HTTP_GET,
        .handler = web_pwm_handler,
        .user_ctx = NULL,
    }; httpd_register_uri_handler(server, &turn);
    httpd_uri_t get_schedule = {
        .uri = "/schedule",
        .method = HTTP_GET,
        .handler = send_schedule,
        .user_ctx = NULL,
    }; httpd_register_uri_handler(server, &get_schedule);

    while(1) {
        vTaskDelay(1000/portTICK_RATE_MS);
    }
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

void schedule_callback(void* params) {
    servo_enq_duty_us(my_servo,1600,1000/portTICK_RATE_MS);
}
esp_err_t web_pwm_handler(httpd_req_t *req) {
    char*  buf;
    size_t buf_len;
    int us, pwm;
    
    buf_len = fmax(httpd_req_get_hdr_value_len(req, "value") + 1,
                  httpd_req_get_hdr_value_len(req, "duration") + 1);
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "value", buf, buf_len) == ESP_OK) {
            sscanf(buf, "%d", &pwm);
        }
        if (httpd_req_get_hdr_value_str(req, "duration", buf, buf_len) == ESP_OK) {
            sscanf(buf, "%d", &us);
        }
        free(buf);
    }
    servo_enq_duty_us(my_servo,pwm,us/portTICK_RATE_MS);
    httpd_resp_send(req, "ok", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t send_schedule(httpd_req_t *req) {
    int p = 0;
    char buff[6*MAX_FEED_IN_A_DAY];
    // attempt to take semaphore 
    assert(my_schedule != NULL);
    if (xSemaphoreTake(my_schedule->smph, (TickType_t) 10) == pdTRUE) {
        for (int i=0; i< my_schedule->size; ++i) {
            sprintf(buff+p,"%02u:%02u,",my_schedule->list[i].hour,my_schedule->list[i].minute);
            p += 6;
        }
        buff[--p] = '\0';
        xSemaphoreGive(my_schedule->smph);
        httpd_resp_send(req, buff, HTTPD_RESP_USE_STRLEN);
        // opt: wake up feed task.
    }
    else {
        httpd_resp_send(req, "dio,cane", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}