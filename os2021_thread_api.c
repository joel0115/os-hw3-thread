#include "os2021_thread_api.h"

struct itimerval Signaltimer;
ucontext_t dispatch_context;
Queue *ready_queue[3], *waiting_queue[8][3], *waiting_for_time_queue, *terminated_queue;
thread_t *running = NULL;
int tid = 1;
int fake_free = 0;

/* required api */
int OS2021_ThreadCreate(char *job_name, char *p_function, char* priority, int cancel_mode)
{
    thread_t *newThread =  CreatenewThread();

    if(!newThread)
    {
        printf("[ERROR] FAIL TO ALLOCATE MEMORY FOR THREAD!\n");
    }

    newThread -> tid = tid++;

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
    newThread -> should_canceled = 0;

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
        func = Function2;
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
    thread_t *to_cancel = NULL;
    Queue *q;
    int flag = 0;
    for(int i = 0; i<3; i++)
    {
        q = ready_queue[i];
        for(thread_t *iter = q -> front; iter != NULL; iter = iter -> next)
        {
            if(!strcmp(job_name, iter -> name))
            {
                to_cancel = iter;
                flag = 1;
                break;
            }
            if(flag)
                break;
        }
    }



    if(!to_cancel)
    {
        for(int i = 0; i<8; i++)
        {
            for(int j = 0; j<3; j++)
            {
                q = waiting_queue[i][j];
                for(thread_t *iter = q -> front; iter != NULL; iter = iter -> next)
                {
                    if(!strcmp(job_name, iter -> name))
                    {
                        to_cancel = iter;
                        flag = 1;
                        break;
                    }

                }
                if(flag)
                    break;
            }
            if(flag)
                break;
        }
    }

    if(!to_cancel)
    {
        q = waiting_for_time_queue;
        for(thread_t *iter = q -> front; iter != NULL; iter = iter -> next)
        {
            if(!strcmp(job_name, iter -> name))
            {
                to_cancel = iter;
                flag = 1;
                break;
            }
        }
    }

    if(to_cancel -> cancel_mode == 0)
    {
        strcpy(to_cancel -> state, "TERMINATED");
        thread_t *prev = NULL;
        thread_t *iter = q -> front;
        while(iter != NULL)
        {
            if(iter != to_cancel)
            {
                prev = iter;
                iter = iter -> next;
            }
            else
            {
                prev -> next = iter -> next;
                iter -> next = NULL;
                break;
            }
        }
        enqueue(terminated_queue, to_cancel);
    }
    else
    {
        to_cancel -> should_canceled = 1;
    }

}

void OS2021_ThreadWaitEvent(int event_id)
{

    thread_t *running_tmp = running;
    strcpy(running_tmp -> state, "WAITING");
    printf("%s wants to wait for event %d\n", running_tmp -> name, event_id);
    running_tmp -> waiting_for = event_id;

    if(!strcmp(running_tmp -> current_priority, "H"))
    {
        dequeue(ready_queue[0]);
        enqueue(waiting_queue[event_id][0], running_tmp);
    }
    else if(!strcmp(running_tmp -> current_priority, "M"))
    {
        strcpy(running_tmp -> current_priority, "H");
        printf("The priority of thread %s is changed from M to H.\n", running_tmp -> name);
        dequeue(ready_queue[1]);
        enqueue(waiting_queue[event_id][1], running_tmp);

    }
    else if(!strcmp(running_tmp -> current_priority, "L"))
    {
        strcpy(running_tmp -> current_priority, "M");
        printf("The priority of thread %s is changed from L to M.\n", running_tmp -> name);
        dequeue(ready_queue[2]);
        enqueue(waiting_queue[event_id][2], running_tmp );
    }
    swapcontext(&(running_tmp -> context), &dispatch_context);
}

void OS2021_ThreadSetEvent(int event_id)
{

    thread_t *running_tmp = running;
    int flag = 0;
    for(int i = 0; i<3; i++)
    {
        for(thread_t *iter = waiting_queue[event_id][i] -> front; iter != NULL; iter = iter -> next)
        {
            if(iter != NULL)
            {
                printf("%s changes the status of %s to READY.\n", running_tmp -> name, iter -> name);
                strcpy(iter -> state, "READY");
                dequeue(waiting_queue[event_id][i]);
                if(!strcmp(iter -> current_priority, "H"))
                {
                    iter -> TQ = 100;
                    enqueue(ready_queue[0], iter);
                }
                else if(!strcmp(iter -> current_priority, "M"))
                {
                    iter -> TQ = 200;
                    enqueue(ready_queue[1], iter);
                }
                else if(!strcmp(iter -> current_priority, "L"))
                {
                    iter -> TQ = 300;
                    enqueue(ready_queue[2], iter);
                }
            }

        }
        if(flag)
        {
            break;
        }
    }
}

void OS2021_ThreadWaitTime(int msec)
{
    thread_t *running_tmp = running;
    running_tmp -> should_wait_time = msec*10;
    strcpy(running_tmp -> state, "WAITING");
    if(!strcmp(running_tmp -> current_priority, "H"))
    {
        dequeue(ready_queue[0]);
    }
    else if(!strcmp(running_tmp -> current_priority, "M"))
    {
        strcpy(running_tmp -> current_priority, "H");
        printf("The priority of thread %s is changed from M to H.\n", running_tmp -> name);
        dequeue(ready_queue[1]);
    }
    else if(!strcmp(running_tmp -> current_priority, "L"))
    {
        strcpy(running_tmp -> current_priority, "M");
        printf("The priority of thread %s is changed from L to M.\n", running_tmp -> name);
        dequeue(ready_queue[2]);
    }
    enqueue(waiting_for_time_queue, running_tmp);
    swapcontext(&(running_tmp -> context), &dispatch_context);

}

void OS2021_DeallocateThreadResource()
{
    clear_queue(terminated_queue);
}

/* cancel point */
void OS2021_TestCancel()
{
    thread_t *running_tmp = running;
    if(running_tmp -> should_canceled == 1)
    {
        strcpy(running_tmp -> state, "TERMINATED");
        if(!strcmp(running_tmp -> current_priority, "H"))
        {
            dequeue(ready_queue[0]);
        }
        if(!strcmp(running_tmp -> current_priority, "M"))
        {
            dequeue(ready_queue[1]);
        }
        if(!strcmp(running_tmp -> current_priority, "L"))
        {
            dequeue(ready_queue[2]);
        }
        enqueue(terminated_queue, running_tmp);
        swapcontext(&(running_tmp -> context),&dispatch_context );
    }
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

void StartTimer()
{
    Signaltimer.it_value.tv_sec = 0;
    Signaltimer.it_value.tv_usec = 10;
    Signaltimer.it_interval.tv_sec = 0;
    Signaltimer.it_interval.tv_usec = 10;

    //for testing
    // Signaltimer.it_value.tv_sec = 1;
    // Signaltimer.it_value.tv_usec = 0;
    // Signaltimer.it_interval.tv_sec = 1;
    // Signaltimer.it_interval.tv_usec = 0;

    if(setitimer(ITIMER_REAL, &Signaltimer, NULL) < 0)
    {
        printf("ERROR SETTING TIME SIGALRM!\n");
    }
}

void Dispatcher()
{
    while(1)
    {
        ResetTimer();
        if(ready_queue[0] -> front != NULL)
        {
            running = ready_queue[0] -> front;
            strcpy(running -> state, "RUNNING");
        }
        else if(ready_queue[1] -> front != NULL)
        {
            running = ready_queue[1] -> front;
            strcpy(running -> state, "RUNNING");
        }
        else if(ready_queue[2] -> front != NULL)
        {
            running = ready_queue[2] -> front;
            strcpy(running -> state, "RUNNING");
        }
        StartTimer();
        swapcontext(&dispatch_context, &(running -> context));
    }

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
    for(int i = 0; i<8; i++)
    {
        for(int j = 0; j<3; j++)
        {
            if(waiting_queue[i][j] -> front == NULL)
            {
                continue;
            }
            for(thread_t *cur = waiting_queue[i][j] -> front; cur != NULL; cur = cur -> next)
            {
                printf("%-8s%-8d%-16s%-8s%-16s%-16s%-8lld%-8lld*\n", "*", cur -> tid,  cur -> name, cur -> state, cur -> base_priority, cur -> current_priority, cur -> ready_time, cur -> waiting_time);
            }
        }
    }

    for(thread_t *cur = waiting_for_time_queue -> front; cur != NULL; cur = cur -> next)
    {
        printf("%-8s%-8d%-16s%-8s%-16s%-16s%-8lld%-8lld*\n", "*", cur -> tid,  cur -> name, cur -> state, cur -> base_priority, cur -> current_priority, cur -> ready_time, cur -> waiting_time);
    }
    for(int i = 0; i<88; i++)
    {
        printf("*");
    }
    printf("\n");
}

void timer_handler(int signal)
{
    ResetTimer();
    running -> TQ -= 10;
    for(int i = 0; i<3; i++)
    {
        for(thread_t *iter = ready_queue[i] -> front; iter != NULL; iter = iter -> next)
        {
            if(!strcmp(iter -> state, "READY"))
            {
                iter -> ready_time += 10;
            }
        }
    }

    for(int i = 0; i<8; i++)
    {
        for(int j = 0; j<3; j++)
        {
            if(waiting_queue[i][j] -> front == NULL)
            {
                continue;
            }
            for(thread_t *cur = waiting_queue[i][j] -> front; cur != NULL; cur = cur -> next)
            {
                cur -> waiting_time += 10;
            }
        }
    }

    thread_t *prev = NULL;
    thread_t *cur = waiting_for_time_queue -> front;
    while(cur != NULL)
    {
        cur -> should_wait_time -= 10;
        cur -> waiting_time += 10;
        if(cur -> should_wait_time == 0)
        {
            if(prev == NULL)
            {
                dequeue(waiting_for_time_queue);
            }
            else
            {
                prev -> next = cur -> next;
                cur -> next = NULL;
            }
            strcpy(cur -> state, "READY");
            if(!strcmp(cur -> current_priority, "H"))
            {
                cur -> TQ = 100;
                enqueue(ready_queue[0], cur);
            }
            else if(!strcmp(cur -> current_priority, "M"))
            {
                cur -> TQ = 200;
                enqueue(ready_queue[1], cur);
            }
            else if(!strcmp(running -> current_priority, "L"))
            {
                cur -> TQ = 300;
                enqueue(ready_queue[2], cur);
            }
        }
        prev = cur;
        cur = cur -> next;
    }

    if(running -> TQ <= 0)
    {
        strcpy(running -> state, "READY");
        if(!strcmp(running -> current_priority, "H"))
        {
            strcpy(running -> current_priority, "M");
            printf("The priority of thread %s is changed from H to M.\n", running -> name);
            dequeue(ready_queue[0]);
            enqueue(ready_queue[1], running);
            running -> TQ = 200;
            swapcontext(&(running -> context), &dispatch_context);
        }
        else if(!strcmp(running -> current_priority, "M"))
        {
            strcpy(running -> current_priority, "L");
            printf("The priority of thread %s is changed from M to L.\n", running -> name);
            dequeue(ready_queue[1]);
            enqueue(ready_queue[2], running);
            running -> TQ = 300;
            swapcontext(&(running -> context), &dispatch_context);
        }
        else
        {
            dequeue(ready_queue[2]);
            enqueue(ready_queue[2], running);
            running -> TQ = 300;
            swapcontext(&(running -> context), &dispatch_context);
        }
    }
    StartTimer();
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
    if(!thread)
    {
        return;
    }
    thread -> next = NULL;

    /* first thread */
    if(q -> rear == NULL)
    {
        q -> front = q -> rear = thread;
        // printf("enqueue successfully\n");
        return;
    }
    else
    {
        q -> rear -> next = thread;
        q -> rear = thread;
        // printf("enqueue successfully\n");
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
        // thread_t *to_delete = q -> front;
        q -> front = q -> front -> next;

        /* when poping the last thread */
        if(q -> front == NULL)
        {
            q -> rear = NULL;
        }
        // to_delete -> next = NULL;
        // printf("dequeue successfully\n");
    }
}

void clear_queue(Queue* q)
{
    thread_t* prev = NULL;
    while(q -> front != NULL)
    {
        prev = q -> front;
        q -> front = q -> front -> next;
        printf("The memory space by %s has been released.\n", prev -> name);
        free(prev -> name);
        free(prev -> entry_function);
        free(prev);
    }
}

void release_queue(Queue *q)
{
    clear_queue(q);
    free(q);
}

thread_t* CreatenewThread()
{
    thread_t *new = (thread_t*)malloc(sizeof(thread_t));
    return new;
}
void StartSchedulingSimulation()
{
    ready_queue[0] = newQueue("H");
    ready_queue[1] = newQueue("M");
    ready_queue[2] = newQueue("L");

    for(int i = 0; i<8; i++)
    {
        waiting_queue[i][0] = newQueue("H");
        waiting_queue[i][1] = newQueue("M");
        waiting_queue[i][2] = newQueue("L");
    }

    waiting_for_time_queue = newQueue("");
    terminated_queue = newQueue("");

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

    // create reclaimer first
    int ret = OS2021_ThreadCreate("reclaimer", "ResourceReclaim", "L", 1);

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

        ret = OS2021_ThreadCreate(name -> valuestring, entry_function -> valuestring, priority -> valuestring, cancel_mode -> valueint);
        if(ret == -1)
        {
            printf("[ERROR] There's no function called %s\n", entry_function -> valuestring);
        }

        // printf("name: %s, entry function: %s, priority: %s, cancel_mode: %s\n", name -> valuestring, entry_function -> valuestring, priority -> valuestring, cancel_mode -> valuestring);
    }

    /* set signal handler*/
    signal(SIGTSTP, print_threads_info);
    signal(SIGALRM, timer_handler);
    // while(1);

    /*Set Timer*/
    Signaltimer.it_interval.tv_usec = 0;
    Signaltimer.it_interval.tv_sec = 0;
    ResetTimer();
    /*Create Context*/
    getcontext(&dispatch_context);
    dispatch_context.uc_stack.ss_sp = malloc(STACK_SIZE);
    dispatch_context.uc_stack.ss_size = STACK_SIZE;
    dispatch_context.uc_stack.ss_flags = 0;

    makecontext(&dispatch_context, Dispatcher,0);
    setcontext(&dispatch_context);

    for(int i = 0; i<3; i++)
    {
        release_queue(ready_queue[i]);
    }

    for(int i = 0; i<8; i++)
    {
        for(int j = 0; j<3; j++)
        {
            release_queue(waiting_queue[i][j]);
        }
    }
    release_queue(waiting_for_time_queue);
    release_queue(terminated_queue);


}
