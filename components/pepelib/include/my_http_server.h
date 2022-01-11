#include "feed_schedule.h"
#include <esp_http_server.h>
#pragma once

// init_feed_schedule must be called before this function
httpd_handle_t start_webserver(schedule_list_handle_t sh); 