#include <esp_http_server.h>
#pragma once

httpd_handle_t start_webserver(); 
void define_debug_callback(httpd_handle_t (*servo_cb)(httpd_req_t *));
void define_update_callback(httpd_handle_t (*update_cb)(httpd_req_t *));
