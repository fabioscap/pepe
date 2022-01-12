#include "my_http_server.h"
#include <esp_log.h>

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

    return server;
}
