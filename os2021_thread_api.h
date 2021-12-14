#ifndef OS2021_API_H
#define OS2021_API_H

#define STACK_SIZE 8192

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "cJSON.h"
#include "function_libary.h"



int OS2021_ThreadCreate(char *job_name, char *p_function, char* priority, int cancel_mode);
void OS2021_ThreadCancel(char *job_name);
void OS2021_ThreadWaitEvent(int event_id);
void OS2021_ThreadSetEvent(int event_id);
void OS2021_ThreadWaitTime(int msec);
void OS2021_DeallocateThreadResource();
void OS2021_TestCancel();


void CreateContext(ucontext_t *, ucontext_t *, void *);
void ResetTimer();
void StartTimer();
void Dispatcher();
void StartSchedulingSimulation();

void print_threads_info(int);
void timer_handler(int);

typedef struct thread_t
{
    int tid;

    char* name;
    char* entry_function;
    char base_priority[100];
    int cancel_mode;

    struct thread_t *next;

    int TQ; //in ms
    char state[200];
    char current_priority[100];
    long long int ready_time;
    long long int waiting_time;
    ucontext_t context;

    int waiting_for;
    long long int should_wait_time;

    int should_canceled;
} thread_t;


typedef struct Queue
{
    thread_t *front;
    thread_t *rear;
    char priority[100];
} Queue;

thread_t* CreatenewThread();
Queue* newQueue(char*);
void enqueue(Queue*, thread_t*);
void dequeue(Queue*);
void clear_queue(Queue*);
#endif
