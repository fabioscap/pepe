#include "feed_schedule.h"
#include "esp_log.h"

void schedule_routine(void* params) {
    schedule_list_handle_t sch = params;
    time_t t_now; 
    time_t day_offset = 24*60*60; // a day offset in seconds.
    while(1) {

        // attempt to take semaphore
        if (xSemaphoreTake(sch->smph, (TickType_t) 10) == pdTRUE) {
            time(&t_now); // fetch current time.
            printf("time: %ld\n", t_now);
            for (uint8_t i = 0; i<sch->size; ++i) {
                queue_element_t* el = sch->list + i;
                printf("actime: %ld\n", el->activation_time);
                if (t_now > el->activation_time && t_now - el->activation_time < TIME_TOLERANCE_SEC) {
                    ESP_LOGI("ciao","ciao");
                    sch->cb_function(NULL);
                    el->activation_time +=  day_offset;
                    break;
                }
            }
            xSemaphoreGive(sch->smph);
        }
        else {
            //
        }
        vTaskDelay(SCHEDULE_ROUTINE_SLEEP_TIME);
    }
    
}
/*
int queue_element_comp(const void* a, const void* b) {

    queue_element_t* _a = (queue_element_t*) a;
    queue_element_t* _b = (queue_element_t*) b;

    if (_a->activation_time > _b->activation_time) return -1;
    if (_a->activation_time < _b->activation_time) return 1;
    return 0;
}
*/
void set_activation_time(schedule_list_handle_t schedule_handler) {
    // set activation times for each timestamp with respect to current time.
    time_t t_now; time(&t_now); // fetch current time.
    struct tm now_human; localtime_r(&t_now,&now_human); // time in human format.

    // get current clock time in seconds
    time_t now_timestamp_in_seconds = now_human.tm_sec +
                                      now_human.tm_min * 60 + 
                                      now_human.tm_hour * 60 * 60;
    time_t this_day = t_now - now_timestamp_in_seconds;
    time_t day_offset = 24*60*60; // a day offset in seconds.

    for (int i=0; i<schedule_handler->size; ++i){
        // two cases:
        // 1- timestamp is greater than current time -> set it this day.
        // 2- timestamp is lower than current time -> set it the next day.
        queue_element_t* timestamp = schedule_handler->list+i;
        time_t feed_timestamp_in_seconds = timestamp->hour*60*60 + timestamp->minute*60;

        if (feed_timestamp_in_seconds > now_timestamp_in_seconds) { 
            //case 1.
            timestamp->activation_time = this_day+feed_timestamp_in_seconds;
        }
        else {
            // case 2.
            timestamp->activation_time = this_day+day_offset+feed_timestamp_in_seconds;
        }
    }
    // sort the timestamps.
    //qsort(schedule_handler->list,schedule_handler->size,sizeof(queue_element_t),queue_element_comp);
}

schedule_list_handle_t init_feed_schedule(void (*cb_function)(void*)) {

    #ifdef FETCH_SCHEDULE_FROM_NVS
    ;
    #endif
    #ifndef FETCH_SCHEDULE_FROM_NVS // use a default schedule.
    queue_element_t s0 = {.hour=20,.minute=39};
    queue_element_t s1 = {.hour=20,.minute=41};
    schedule_handler = (schedule_list_handle_t)malloc(sizeof(schedule_list_t));
    schedule_handler->list[0] = s0;
    schedule_handler->list[1] = s1;
    schedule_handler->size = 2;
    #endif
    set_activation_time(schedule_handler);
    printf("ACTIME: %ld\n",schedule_handler->list[0].activation_time);
    schedule_handler->cb_function = cb_function;

    //update_feed_schedule(schedule_handler);

    // semaphore for overwriting schedule.
    schedule_handler->smph = xSemaphoreCreateMutex();

    // create task
    TaskHandle_t schedule_task = NULL;
    xTaskCreate(schedule_routine,"schedule_routine",4096,schedule_handler,SCHEDULE_TASK_PRIORITY,&schedule_task);
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