#include "os2021_thread_api.h"

struct itimerval Signaltimer;
ucontext_t dispatch_context;
ucontext_t timer_context;
Queue *ready_queue[3];
int tid;

/* required api */
int OS2021_ThreadCreate(char *job_name, char *p_function, char* priority, int cancel_mode)
{
    thread_t *newThread = (thread_t*)malloc(sizeof(thread_t));
    //for pushing to github
    free(newThread);
    // cppcheck-suppress memleak
    if(!newThread)
    {
        printf("[ERROR] FAIL TO ALLOCATE MEMORY FOR THREAD!\n");
    }

    newThread -> tid = tid;

    newThread -> name = (char*)malloc(sizeof(job_name));
    newThread -> entry_function = (char*)malloc(sizeof(p_function));
    if(!newThread -> name || !newThread -> entry_function)
    {
        printf("[ERROR] FAIL TO ALLOCATE MEMORY FOR THREAD!\n");
    }
    /* basic thread info */
    strcpy(newThread -> name, job_name);
    strcpy(newThread -> entry_function, p_function);
    strcpy(newThread -> base_priority, priority);
    strcpy(newThread -> current_priority, priority);
    newThread -> cancel_mode = cancel_mode;
    newThread -> next = NULL;
    strcpy(newThread -> state, "READY");
    newThread -> ready_time = 0;
    newThread -> waiting_time = 0;


    /* construct context */
    getcontext(&(newThread -> context));
    newThread -> context.uc_stack.ss_sp = malloc(STACK_SIZE);
    newThread -> context.uc_stack.ss_size = STACK_SIZE;
    newThread -> context.uc_stack.ss_flags = 0;

    void (*func)(void) = NULL;
    if(!strcmp(newThread -> entry_function, "Function1"))
    {
        func = Function1;
    }
    else if(!strcmp(newThread -> entry_function, "Function2"))
    {
        func = Function1;
    }
    else if(!strcmp(newThread -> entry_function, "Function3"))
    {
        func = Function3;
    }
    else if(!strcmp(newThread -> entry_function, "Function4"))
    {
        func = Function4;
    }
    else if(!strcmp(newThread -> entry_function, "Function5"))
    {
        func = Function5;
    }
    else if(!strcmp(newThread -> entry_function, "ResourceReclaim"))
    {
        func = ResourceReclaim;
    }
    else
    {
        return -1;
    }
    makecontext(&(newThread -> context),func,0);

    /* push to corresponding level queue and set the time quantum */
    if(!strcmp(priority, "H"))
    {
        newThread -> TQ = 100;
        enqueue(ready_queue[0], newThread);
    }
    else if(!strcmp(priority, "M"))
    {
        newThread -> TQ = 200;
        enqueue(ready_queue[1], newThread);
    }
    else if(!strcmp(priority, "L"))
    {
        newThread -> TQ = 300;
        enqueue(ready_queue[2], newThread);
    }

    return tid;
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

/* signal hanlders */
void print_threads_info(int signal)
{

    printf("\n");
    for(int i = 0; i<88; i++)
    {
        printf("*");
    }
    printf("\n");

    printf("%-8s%-8s%-16s%-8s%-16s%-16s%-8s%-8s*\n", "*", "TID", "Name", "State", "B_Priority", "C_Priority", "Q_Time", "W_Time");
    for(int i = 0; i<3; i++)
    {
        if(ready_queue[i] -> front == NULL)
        {
            continue;
        }
        for(thread_t *cur = ready_queue[i] -> front; cur != NULL; cur = cur -> next)
        {
            printf("%-8s%-8d%-16s%-8s%-16s%-16s%-8lld%-8lld*\n", "*", cur -> tid,  cur -> name, cur -> state, cur -> base_priority, cur -> current_priority, cur -> ready_time, cur -> waiting_time);
        }
    }
    for(int i = 0; i<88; i++)
    {
        printf("*");
    }
    printf("\n");
}


/* queue operations */
Queue* newQueue(char* priority)
{
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if(!q)
    {
        printf("[ERROR] FAIL TO ALLOCATE MEMORY FOR QUEUE!\n");
    }
    q -> front = q -> rear = NULL;
    strcpy(q -> priority, priority);
    return q;
}

void enqueue(Queue *q, thread_t *thread)
{
    /* first thread */
    if(q -> rear == NULL)
    {
        q -> front = q -> rear = thread;
        return;
    }
    else
    {
        q -> rear -> next = thread;
        q -> rear = thread;
        return;
    }
}

void dequeue(Queue *q)
{
    /* empty queue */
    if(q -> front == NULL)
    {
        return;
    }
    else
    {
        thread_t *to_delete = q -> front;
        q -> front = q -> front -> next;

        /* when poping the last thread */
        if(q -> front == NULL)
        {
            q -> rear = NULL;
        }
        free(to_delete);
    }
}

void StartSchedulingSimulation()
{
    ready_queue[0] = newQueue("H");
    ready_queue[1] = newQueue("M");
    ready_queue[2] = newQueue("L");

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
        else
        {
            tid++;
        }
        // printf("name: %s, entry function: %s, priority: %s, cancel_mode: %s\n", name -> valuestring, entry_function -> valuestring, priority -> valuestring, cancel_mode -> valuestring);
    }

    /* set signal handler*/
    signal(SIGTSTP, print_threads_info);
    // while(1);

    /*Set Timer*/
    Signaltimer.it_interval.tv_usec = 0;
    Signaltimer.it_interval.tv_sec = 0;
    ResetTimer();
    /*Create Context*/
    CreateContext(&dispatch_context, &timer_context, &Dispatcher);
    // setcontext(&dispatch_context);
}
