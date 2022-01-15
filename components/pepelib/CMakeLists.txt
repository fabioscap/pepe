set(srcs "src/feed_schedule.c" "src/wifi_setup.c" "src/my_http_server" "src/drive_servo.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server
                       REQUIRES mdns
                       EMBED_TXTFILES "index/index.html"
)