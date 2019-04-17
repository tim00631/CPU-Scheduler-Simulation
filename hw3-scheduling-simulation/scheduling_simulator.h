#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include "task.h"
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
enum TASK_STATE
{
	TASK_RUNNING,
	TASK_READY,
	TASK_WAITING,
	TASK_TERMINATED,
	TASK_DELETED
};

struct task
{
	ucontext_t ctx;
	char TASK_NAME[10];
	char TIME_QUANTUM[10];
	char PRIORITY[10];
	int interrupt_flag;
	int TASK_STATE;
	int Queueing_Time ;
	int restart_flag;
};
struct element{
	int pid;
	int msec_10;
	//int interrupt_flag;
};
struct node
{
	//Linked list struct node
	int pid;
	//struct task *task; //task information(pointer)
	struct node *next;
};
struct wait_node
{
	struct element element;
	struct wait_node *next;
};
void Waiting_Q_Push(struct element element);
struct element Waiting_Q_Pop(void);
int Waiting_Q_isEmpty(void);
void H_Push(int pid);			
int H_Pop(void);
void H_Remove(int pid);
int H_isEmpty(void);
void L_Push(int pid);			
int L_Pop(void);
void L_Remove(int pid);
int L_isEmpty(void);
void shell();
void token(char *command, char *cutstring[]);
void command_selector(char *command, char *cutstring[]);
void Ctrl_Z_Handler(int signal_number);
void scheduler();
void Task_Remove(int pid);
void Alarm_Handler(int signal_number);
int set_timer();
int stop_timer();
void set_scheduler_context();
void set_shell_context();
int combine_context_function();
void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);
#endif
