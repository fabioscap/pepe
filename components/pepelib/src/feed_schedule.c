#include "feed_schedule.h"


schedule_list_handle_t init_feed_schedule(void (*cb_function)(void*)) {

    #ifdef FETCH_SCHEDULE_FROM_NVS
    ;
    #endif
    #ifndef FETCH_SCHEDULE_FROM_NVS // use a default schedule.
    queue_element_t s0 = {.hour=5,.minute=30};
    queue_element_t s1 = {.hour=22,.minute=40};
    schedule_handler = (schedule_list_handle_t)malloc(sizeof(schedule_list_t));
    schedule_handler->list[0] = s0;
    schedule_handler->list[1] = s1;
    schedule_handler->size = 2;
    #endif
    // semaphore for overwriting schedule.
    schedule_handler->smph = xSemaphoreCreateMutex();
    
    // start feed routine.
    return schedule_handler;
}

void print_feed_schedule(queue_element_t* fs, uint8_t n) {
    char buff[n*6 + 2]; //HH:MM -> 6 chars
    int p = 0;
    buff[p++] = '[';
    for (int i=0; i<n; ++i) {
        sprintf(buff+p,"%02u:%02u ",fs[i].hour,fs[i].minute);
        p += 6;
    }
    buff[--p] = ']';
    buff[++p] = '\n';
    buff[++p] = 0;
    printf(buff);
}