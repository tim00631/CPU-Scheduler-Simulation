#include "scheduling_simulator.h"
#define NUMCONTEXTS 40		 /* how many contexts to make */
#define STACKSIZE 1024 * 128 /* stack size */
char *STATE_TABLE[] = {
	"TASK_RUNNING",
	"TASK_READY",
	"TASK_WAITING",
	"TASK_TERMINATED",
	"TASK_DELETED"};

struct wait_node *Waiting_Q_HEAD = NULL;
struct wait_node *Waiting_Q_TAIL = NULL;
struct node *H_HEAD = NULL;
struct node *H_TAIL = NULL;
struct node *L_HEAD = NULL;
struct node *L_TAIL = NULL;
struct itimerval timer;
int Waiting_Q_NUM = 0;
int H_NUM = 0;
int L_NUM = 0;
int i = 0;
int PID = 1;
int current_task_pid = 0;		/* a pointer to the current_task */
char schedule_stack[STACKSIZE]; //stack size = 128KB
char shell_stack[STACKSIZE];	//stack size = 128KB
struct task task[100];//pid is index
//struct wait_element Wait_Queue[100];
ucontext_t scheduler_context;
ucontext_t shell_context;
//ucontext_t contexts[NUMCONTEXTS]; /* store our context info */
int main()
{
	signal(SIGALRM, Alarm_Handler);  //time interrupt signal
	signal(SIGTSTP, Ctrl_Z_Handler); //Ctrl-Z interupt signal
	set_shell_context();
	setcontext(&shell_context);
	//shell();
	return 0;
}

void shell()
{
	//printf("hello world!\n");
	stop_timer();
	set_scheduler_context();
	//Ctrl-Z handle to current
	char command[50];
	while (1)
	{
		fprintf(stdout, "$");
		fflush(stdout);
		i = 0;
		char *cutstring[10];
		fgets(command, sizeof(command), stdin);
		token(command, cutstring);
		command_selector(command, cutstring);
	}
}
void token(char *command, char *cutstring[])
{
	command = strtok(command, "\n"); //cut out the end character
	char *pch = NULL;
	pch = strtok(command, " ");
	while (pch != NULL)
	{
		cutstring[i] = pch;
		//printf("cutstring[%d]:%s\n", i, cutstring[i]);
		i++;
		pch = strtok(NULL, " ");
	}
	//printf("%d=======\n", i);
}
void command_selector(char *command, char *cutstring[])
{
	//printf("i:%d", i);
	if (!strcmp(cutstring[0], "add"))
	{
		int push_flag = 0;
		if (cutstring[1] != NULL)
		{
			strcpy(task[PID].TASK_NAME, cutstring[1]);
			//new_task->PID = PID;
		}
		else
		{
			printf("your TASK_NAME is empty");
		}

		if (i > 2)
		{
			if (!strcmp(cutstring[2], "-t"))
			{
				//printf("get -t \n");
				strcpy(task[PID].TIME_QUANTUM, cutstring[3]);
			}
		}
		else
		{
			//printf("not get -t\n");
			//printf("new_task->TIME_QUANTUM = S\nnew_task->PRIORITY = L\n");
			strcpy(task[PID].TIME_QUANTUM, "S");
			// strcpy(new_task->PRIORITY, "L");
		}
		if (i > 4)
		{
			if (!strcmp(cutstring[4], "-p"))
			{
				//printf("get -p \n");
				strcpy(task[PID].PRIORITY, cutstring[5]);
			}
		}
		else
		{
			//printf("not get -p\n");
			strcpy(task[PID].PRIORITY, "L");
		}
		if (!strcmp(task[PID].PRIORITY, "H"))
		{ //if priority is High
			push_flag = 1;
		}
		combine_context_function(PID);
		task[PID].Queueing_Time = 0;
		task[PID].TASK_STATE = TASK_READY; //TASK_READY
		if (push_flag)
		{									   //check which queue to be push
			H_Push(PID);
		}
		else
		{
			task[PID].TASK_STATE = TASK_READY; //TASK_READY
			L_Push(PID);
			//printf("%p\n", task[PID].ctx.uc_link);
			//setcontext(&task[PID].ctx);
		}
		//printf("add last PID:%d\n", PID);
		PID++;
	}
	else if (!strcmp(cutstring[0], "remove"))
	{
		//strcpy(OPTION, "remove");
		int remove_pid = 0;
		if (cutstring[1] != NULL)
		{
			remove_pid = atoi(cutstring[1]);
			//remove
			if(!strcmp(task[remove_pid].PRIORITY,"H")){
				H_Remove(remove_pid);
			}
			else{
				L_Remove(remove_pid);
			}
			Task_Remove(remove_pid);
		}
		else
		{
			printf("you didn't input a pid\n");
		}
	}
	else if (!strcmp(cutstring[0], "ps"))
	{
		for (int i = 1; i < PID; i++){
			printf("%d %s %s %d %s %s\n", i, task[i].TASK_NAME, STATE_TABLE[task[i].TASK_STATE],task[i].Queueing_Time ,task[i].PRIORITY, task[i].TIME_QUANTUM);
		}
	}
	else if (!strcmp(cutstring[0], "start"))
	{
		if (current_task_pid !=0 && task[current_task_pid].restart_flag == 1 && task[current_task_pid].TASK_STATE == TASK_RUNNING)
		{
			printf("restart...\n");
			printf("simulating...\n");
			//task[current_task_pid].TASK_STATE = TASK_RUNNING;
			task[current_task_pid].restart_flag = 0;
			set_timer(task[current_task_pid].TIME_QUANTUM);
			setcontext(&task[current_task_pid].ctx);
		}
		//set_scheduler_context();
		printf("simulating...\n");
		setcontext(&scheduler_context);
	}
	else
	{
		printf("you suck a bad request!!!\n");
	}
}

void Waiting_Q_Push(struct element element) //Linked list Push
{
	if (Waiting_Q_HEAD == NULL)
	{
		Waiting_Q_HEAD = (struct wait_node *)malloc(sizeof(struct wait_node));
		Waiting_Q_HEAD->element = element;
		Waiting_Q_HEAD->next = NULL;
		Waiting_Q_TAIL = Waiting_Q_HEAD;
	}
	else
	{
		struct wait_node *ptr = (struct wait_node *)malloc(sizeof(struct wait_node));
		ptr->element = element;
		ptr->next = NULL;
		Waiting_Q_TAIL->next = ptr;
		Waiting_Q_TAIL = ptr;
	}
	Waiting_Q_NUM++;
}

struct element Waiting_Q_Pop(void) //Linked list pop
{
	struct wait_node *ptr = Waiting_Q_HEAD;
	struct element result = ptr->element;
	Waiting_Q_HEAD = ptr->next;
	free(ptr);
	Waiting_Q_NUM--;
	return result;
}
void Waiting_Q_Delete(struct wait_node* ptr){
	struct wait_node *temp = ptr;
	struct wait_node *prev = Waiting_Q_HEAD;
	if(temp == Waiting_Q_HEAD){ //delete head node
		Waiting_Q_HEAD = Waiting_Q_HEAD->next;
		ptr = Waiting_Q_HEAD;
		free(temp);
		//printf("free head\n");
		Waiting_Q_NUM--;
	}
	else { //delete medium node
		while(prev->next != ptr && prev->next != NULL){
		prev = prev->next;
		}
		prev->next = ptr->next;
		free(temp);
		//printf("free temp\n");
		Waiting_Q_NUM--;
	}
}

int Waiting_Q_isEmpty(void) // Linked list isEmpty
{
	if (Waiting_Q_NUM == 0)
		return 1;
	else
		return 0;
}

void H_Push(int pid) //Linked list Push
{
	if (H_HEAD == NULL)
	{
		H_HEAD = (struct node *)malloc(sizeof(struct node));
		H_HEAD->pid = pid;
		H_HEAD->next = NULL;
		H_TAIL = H_HEAD;
	}
	else
	{
		struct node *ptr = (struct node *)malloc(sizeof(struct node));
		ptr->pid = pid;
		ptr->next = NULL;
		H_TAIL->next = ptr;
		H_TAIL = ptr;
	}
	H_NUM++;
}

int H_Pop(void) //Linked list pop
{
	struct node *ptr = H_HEAD;
	int result = ptr->pid;
	H_HEAD = ptr->next;
	free(ptr);
	H_NUM--;
	return result;
}
void H_Remove(int pid){
	if(H_isEmpty()){
		return;
	}
	if(H_HEAD->pid == pid){ //if head node is the node to be removed
		H_Pop();
		return;
	}
	struct node *ptr = H_HEAD;
	struct node *prev = NULL;
	while (ptr != NULL && ptr->pid != pid)
	{
		prev = ptr;
		ptr = ptr->next;
	}
	if(ptr == NULL){ //not found this pid
		printf("Not found this PID\n");
		return;
	}
	else{
		prev->next = ptr->next;
		free(ptr);
		H_NUM--;
	}
}
int H_isEmpty(void) // Linked list isEmpty
{
	if (H_NUM == 0)
		return 1;
	else
		return 0;
}

void L_Push(int pid) //Linked list Push
{
	if (L_HEAD == NULL)
	{
		L_HEAD = (struct node *)malloc(sizeof(struct node));
		L_HEAD->pid = pid;
		L_HEAD->next = NULL;
		L_TAIL = L_HEAD;
	}
	else
	{
		struct node *ptr = (struct node *)malloc(sizeof(struct node));
		ptr->pid = pid;
		ptr->next = NULL;
		L_TAIL->next = ptr;
		L_TAIL = ptr;
	}
	L_NUM++;
}

int L_Pop(void) //Linked list pop
{
	struct node *ptr = L_HEAD;
	int result = ptr->pid;
	L_HEAD = ptr->next;
	free(ptr);
	L_NUM--;
	return result;
}
void L_Remove(int pid){
	if (L_isEmpty())
	{
		return;
	}
	if (L_HEAD->pid == pid)
	{ //if head node is the node to be removed
		L_Pop();
		// printf("pop head node\n");
		return;
	}
	struct node *ptr = L_HEAD;
	struct node *prev = NULL;
	while (ptr != NULL && ptr->pid != pid)
	{
		prev = ptr;
		ptr = ptr->next;
	}
	if (ptr == NULL)
	{ //not found this pid
		return;
	}
	else
	{
		//printf("middle node\n");
		prev->next = ptr->next;
		free(ptr);
		L_NUM--;
	}
}
int L_isEmpty(void) // Linked list isEmpty
{
	if (L_NUM == 0)
		return 1;
	else
		return 0;
}

void Ctrl_Z_Handler(int signal_number)
{
	//save cur_task
	printf("\n");
	stop_timer();
	//task[current_task_pid].TASK_STATE = TASK_RUNNING;
	task[current_task_pid].restart_flag = 1;
	swapcontext(&task[current_task_pid].ctx, &shell_context);
}
void scheduler(void)
{
	stop_timer();
	if (current_task_pid != 0 && task[current_task_pid].TASK_STATE == TASK_RUNNING && task[current_task_pid].restart_flag != 1)
	{
		task[current_task_pid].TASK_STATE = TASK_TERMINATED;
	}
	//printf("TASK_NAME:%sTASK_STATE:%s\n", task[current_task_pid].TASK_NAME,STATE_TABLE[task[current_task_pid].TASK_STATE]);
	for (int i = 1; i < PID; i++)
	{
		if(task[i].TASK_STATE == TASK_READY){
			task[i].Queueing_Time += 10;
		}
	}
	struct wait_node *ptr;
	struct wait_node *buffer[20];
	int index = 0;
	for (ptr = Waiting_Q_HEAD; ptr != NULL; ptr = ptr->next)
	{
		//printf("pid %d:msec %d\n", ptr->element.pid, ptr->element.msec_10);
		if(task[ptr->element.pid].interrupt_flag == 1){ //be waken up
			buffer[index] = ptr; 
			index++;
		}
		else{ //self counting
			ptr->element.msec_10 -= 10;
			if (ptr->element.msec_10 <= 0){
				//hw_wakeup_pid(ptr->element.pid);
				//Waiting_Q_Delete(ptr);
				buffer[index] = ptr;
				index++;
			}
		}
	}
	//printf("index:%d\n", index);
	for (int i = 0; i < index;i++){
		if (task[buffer[i]->element.pid].interrupt_flag == 1){ //if is interrupt
			//printf("Waiting_Q_Delete is by interrupt\n");
		}
		else{ //self counting
			//printf("Waiting_Q_Delete is by self counting\n");
			if (!strcmp(task[i].PRIORITY, "H"))
			{
				H_Push(buffer[i]->element.pid);
			}
			else
			{
				L_Push(buffer[i]->element.pid);
			}
		}
		Waiting_Q_Delete(buffer[i]);
	}
	
	//printf("entering scheduler...\n");
	while (!H_isEmpty())
	{
		current_task_pid = H_Pop();
		//printf("PID:%d TASK_NAME:%s TIME_QUANTUM:%s PRIORITY:%s\n", current_task_pid, task[current_task_pid].TASK_NAME, task[current_task_pid].TIME_QUANTUM, task[current_task_pid].PRIORITY);
		task[current_task_pid].TASK_STATE = TASK_RUNNING;
		set_timer(task[current_task_pid].TIME_QUANTUM);
		setcontext(&task[current_task_pid].ctx);
		//free(task);
	}
	while (!L_isEmpty())
	{
		current_task_pid = L_Pop();
		//printf("PID:%d TASK_NAME:%s TIME_QUANTUM:%s PRIORITY:%s\n", current_task_pid, task[current_task_pid].TASK_NAME, task[current_task_pid].TIME_QUANTUM, task[current_task_pid].PRIORITY);
		task[current_task_pid].TASK_STATE = TASK_RUNNING;
		set_timer(task[current_task_pid].TIME_QUANTUM);
		//printf("timer setting completed\n");
		setcontext(&task[current_task_pid].ctx);
	}
	while(!Waiting_Q_isEmpty())
	{
		setcontext(&scheduler_context);
	}
	//printf("WaitingQ is empty\n");
	//printf("Schedulerdone\n");
	current_task_pid = 0;
}
void Task_Remove(int pid)
{
	if(pid == 0 || pid >= PID){
		printf("Error: Your request is a illegal pid_number.\n");
		return;
	}
	strcpy(task[pid].TASK_NAME, "-----");
	task[pid].TASK_STATE = TASK_DELETED;
	task[pid].Queueing_Time = 0;
	strcpy(task[pid].PRIORITY, "");
	strcpy(task[pid].TIME_QUANTUM, "");
}
void Alarm_Handler(int signal_number)
{
	fflush(stdout);
	task[current_task_pid].TASK_STATE = TASK_READY;
	if (!strcmp(task[current_task_pid].PRIORITY, "H"))
	{
		H_Push(current_task_pid);
	}
	else
	{
		L_Push(current_task_pid);
	}
	swapcontext(&task[current_task_pid].ctx, &scheduler_context);
}

int set_timer(char *TIME_QUANTUM)
{
	int num;
	if (!strcmp(TIME_QUANTUM, "L"))
	{
		num = 20000;
	}
	else
	{
		num = 10000;
	}
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = num;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
	return 0;
}

int stop_timer()
{
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
	return 0;
}

void set_shell_context()
{
	getcontext(&shell_context);
	shell_context.uc_stack.ss_sp = malloc(STACKSIZE);			  //指定stack
	shell_context.uc_stack.ss_size = STACKSIZE; //指定stack大小
	shell_context.uc_stack.ss_flags = 0;
	shell_context.uc_link = &shell_context;
	makecontext(&shell_context, (void (*)(void))shell, 0); //修改context指向shell
}

void set_scheduler_context()
{
	getcontext(&scheduler_context);
	scheduler_context.uc_stack.ss_sp = malloc(STACKSIZE);			 //指定stack
	scheduler_context.uc_stack.ss_size = STACKSIZE; //指定stack大小
	scheduler_context.uc_stack.ss_flags = 0;
	scheduler_context.uc_link = &shell_context;
	makecontext(&scheduler_context, (void (*)(void))scheduler, 0); //修改context指向schedule
}
int combine_context_function(){
	getcontext(&task[PID].ctx);
	task[PID].ctx.uc_stack.ss_sp = malloc(STACKSIZE); //指定stack
	task[PID].ctx.uc_stack.ss_size = STACKSIZE;		  //指定stack大小
	task[PID].ctx.uc_stack.ss_flags = 0;
	task[PID].ctx.uc_link = &scheduler_context;
	if (!strcmp(task[PID].TASK_NAME, "task1"))
	{
		makecontext(&task[PID].ctx, (void (*)(void))task1, 0); //修改context指向task1
	}
	else if (!strcmp(task[PID].TASK_NAME, "task2"))
	{
		makecontext(&task[PID].ctx, (void (*)(void))task2, 0); //修改context指向task2
	}
	else if (!strcmp(task[PID].TASK_NAME, "task3"))
	{
		makecontext(&task[PID].ctx, (void (*)(void))task3, 0); //修改context指向task3
	}
	else if (!strcmp(task[PID].TASK_NAME, "task4"))
	{
		makecontext(&task[PID].ctx, (void (*)(void))task4, 0); //修改context指向task4
	}
	else if (!strcmp(task[PID].TASK_NAME, "task5"))
	{
		makecontext(&task[PID].ctx, (void (*)(void))task5, 0); //修改context指向task5
	}
	else if (!strcmp(task[PID].TASK_NAME, "task6"))
	{
		makecontext(&task[PID].ctx, (void (*)(void))task6, 0); //修改context指向task6
	}
	else
	{
		printf("make context error\n");
		return -1;
	}

	return PID;
}
void hw_suspend(int msec_10)
{
	task[current_task_pid].TASK_STATE = TASK_WAITING;
	struct element element;
	element.pid = current_task_pid;
	element.msec_10 = msec_10*10;
	Waiting_Q_Push(element);
	swapcontext(&task[current_task_pid].ctx,&scheduler_context);
	return;
}
void hw_wakeup_pid(int pid)
{
	if(task[pid].TASK_STATE == TASK_WAITING){
		task[pid].TASK_STATE = TASK_READY;
		task[pid].interrupt_flag = 1;
		if (!strcmp(task[pid].PRIORITY, "H"))
		{
			H_Push(pid);
		}
		else
		{
			L_Push(pid);
		}
	}
	return;
}

int hw_wakeup_taskname(char *task_name)
{
	int num = 0;
	for (int i = 0; i < PID; i++)
	{
		if(!strcmp(task[i].TASK_NAME,task_name) && task[i].TASK_STATE == TASK_WAITING){
			task[i].interrupt_flag = 1;
			if (!strcmp(task[i].PRIORITY, "H"))
			{
				H_Push(i);
			}
			else
			{
				L_Push(i);
			}
			num++;
		}
	}
		return num;
}

int hw_task_create(char *task_name)
{
	strcpy(task[PID].TASK_NAME, task_name);
	int flag = combine_context_function();
	if (flag == -1)
	{
		return flag;
	}
	else{
		strcpy(task[PID].TIME_QUANTUM, "S");
		strcpy(task[PID].PRIORITY, "L");
		task[PID].TASK_STATE = TASK_READY;
		L_Push(PID);
		PID++;
		return flag; // the pid of created task name
	}
}
