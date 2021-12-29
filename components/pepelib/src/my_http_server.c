#include "my_http_server.h"
#include <esp_http_server.h>


httpd_handle_t start_webserver(schedule_list_handler_t sh) {
    // default http server configuration.
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // server handle.
    httpd_handle_t server = NULL;
    // start server.
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    return server;
}