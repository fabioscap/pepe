#include "my_http_server.h"
#include "drive_servo.h"
#include <esp_log.h>

esp_err_t web_pwm_handler(httpd_req_t *req) {
    char*  buf;
    size_t buf_len;
    buf_len = httpd_req_get_hdr_value_len(req, "value") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "value", buf, buf_len) == ESP_OK) {
            int pwm;
            sscanf(buf, "%d", &pwm);
            ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, pwm));
        }
        free(buf);
    }
    httpd_resp_send(req, "ok", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t send_schedule(httpd_req_t *req) {
    int p = 0;
    char buff[6*MAX_FEED_IN_A_DAY];
    // attempt to take semaphore 
    assert(schedule_handler != NULL);
    if (xSemaphoreTake(schedule_handler->smph, (TickType_t) 10) == pdTRUE) {
        for (int i=0; i< schedule_handler->size; ++i) {
            sprintf(buff+p,"%02u:%02u,",schedule_handler->list[i].hour,schedule_handler->list[i].minute);
            p += 6;
        }
        buff[--p] = '\0';
        xSemaphoreGive(schedule_handler->smph);
        httpd_resp_send(req, buff, HTTPD_RESP_USE_STRLEN);
        // opt: wake up feed task.
    }
    else {
        httpd_resp_send(req, "dio,cane", HTTPD_RESP_USE_STRLEN);
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

httpd_handle_t start_webserver(schedule_list_handler_t sh) {
    // default http server configuration.
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // server handle.
    httpd_handle_t server = NULL;
    // start server.
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t get_index = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL,
    };

    httpd_uri_t get_schedule = {
        .uri = "/schedule",
        .method = HTTP_GET,
        .handler = send_schedule,
        .user_ctx = NULL,
    };
    httpd_uri_t turn = {
        .uri = "/pwm",
        .method = HTTP_GET,
        .handler = web_pwm_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &turn);
    httpd_register_uri_handler(server, &get_index);
    httpd_register_uri_handler(server, &get_schedule);
    
    return server;
}