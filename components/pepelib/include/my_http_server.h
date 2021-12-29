#include "feed_schedule.h"
#include <esp_http_server.h>
#pragma once

httpd_handle_t start_webserver(schedule_list_handler_t sh); 