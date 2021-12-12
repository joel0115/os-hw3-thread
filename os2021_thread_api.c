#include "os2021_thread_api.h"

struct itimerval Signaltimer;
ucontext_t dispatch_context;
ucontext_t timer_context;

int OS2021_ThreadCreate(char *job_name, char *p_function, char* priority, int cancel_mode)
{
    return -1;
}

void OS2021_ThreadCancel(char *job_name)
{

}

void OS2021_ThreadWaitEvent(int event_id)
{

}

void OS2021_ThreadSetEvent(int event_id)
{

}

void OS2021_ThreadWaitTime(int msec)
{

}

void OS2021_DeallocateThreadResource()
{

}

void OS2021_TestCancel()
{

}

void CreateContext(ucontext_t *context, ucontext_t *next_context, void *func)
{
    getcontext(context);
    context->uc_stack.ss_sp = malloc(STACK_SIZE);
    context->uc_stack.ss_size = STACK_SIZE;
    context->uc_stack.ss_flags = 0;
    context->uc_link = next_context;
    makecontext(context,(void (*)(void))func,0);
}

void ResetTimer()
{
    Signaltimer.it_value.tv_sec = 0;
    Signaltimer.it_value.tv_usec = 0;
    if(setitimer(ITIMER_REAL, &Signaltimer, NULL) < 0)
    {
        printf("ERROR SETTING TIME SIGALRM!\n");
    }
}

void Dispatcher()
{

}

void StartSchedulingSimulation()
{
    /*Parse json file*/
    FILE* json_file = fopen("init_threads.json", "rb");
    char* json_str = NULL;

    /*Read the entire file as a string*/
    if(!json_file)
    {
        printf("ERROR OPENING THE JSON FILE!\n");
        fclose(json_file);
        return;
    }
    else
    {
        int len_byte;
        fseek(json_file, 0, SEEK_END);
        len_byte = ftell(json_file);
        fseek(json_file, 0, SEEK_SET);
        json_str = malloc(len_byte);
        if(!json_str)
        {
            printf("ERROR ALLOCATING MEMORY SPACE FOR JSON_STRING!\n");
            fclose(json_file);
            return;
        }
        else
        {
            fread(json_str, 1, len_byte, json_file);
        }
    }
    fclose(json_file);
    // printf("%s\n", json_str);

    /*Start parsing*/
    cJSON *parsed = cJSON_Parse(json_str);
    if(!parsed)
    {
        printf("ERROR PARSING JSON STRING!\n");
    }
    /*Get the thread list*/
    cJSON *threads = cJSON_GetObjectItem(parsed, "Threads");
    cJSON *thread = NULL;
    /*Iterate through every thread*/
    cJSON_ArrayForEach(thread, threads)
    {
        cJSON *name = cJSON_GetObjectItem(thread, "name");
        cJSON *entry_function = cJSON_GetObjectItem(thread, "entry function");
        cJSON *priority = cJSON_GetObjectItem(thread, "priority");
        cJSON *cancel_mode = cJSON_GetObjectItem(thread, "cancel mode");

        int ret = OS2021_ThreadCreate(name -> valuestring, entry_function -> valuestring, priority -> valuestring, cancel_mode -> valueint);
        if(ret == -1)
        {
            printf("[ERROR] There's no function called %s\n", entry_function -> valuestring);
        }
        // printf("name: %s, entry function: %s, priority: %s, cancel_mode: %s\n", name -> valuestring, entry_function -> valuestring, priority -> valuestring, cancel_mode -> valuestring);
    }

    /*Set Timer*/
    Signaltimer.it_interval.tv_usec = 0;
    Signaltimer.it_interval.tv_sec = 0;
    ResetTimer();
    /*Create Context*/
    CreateContext(&dispatch_context, &timer_context, &Dispatcher);
    // setcontext(&dispatch_context);
}
