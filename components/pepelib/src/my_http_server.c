#include "my_http_server.h"
#include <esp_log.h>
/*
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


*/
esp_err_t get_handler(httpd_req_t *req)
{   
    // fetch html page
    extern const uint8_t index_html[] asm("index_html");
    httpd_resp_send(req, (char*)index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_handle_t start_webserver() {
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
    }; httpd_register_uri_handler(server, &get_index);
    /*



    */
    return server;
}
