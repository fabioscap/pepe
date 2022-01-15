#pragma once 

#include <esp_netif.h>
#include <esp_wifi.h>
#include "freertos/event_groups.h"
#include <esp_sntp.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include "mdns.h"

#define HOSTNAME "pepe"

void wifi_setup(char* ssid, char* pwd);
