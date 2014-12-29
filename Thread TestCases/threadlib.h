#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>

#include <time.h>
#include <unistd.h>

#define true 1
#define false 0
#define Stack_Size 4096
#define sec_1 1000000
#define BILLION 1000000000

struct thread_status{
   
    int id;
       unsigned int no_of_bursts,avr_exec_time;
      unsigned total_exec_time;
       enum {RUNNING, READY, SUSPENDED, TERMINATED} state;
       
};

typedef struct thread_status status;

typedef void (*fun)(void);
typedef void* (*funWithArg)(void*);

struct thread{

    int tid;           
    sigjmp_buf env;
    status stat;       
    short hasArg;
    fun fun;
    funWithArg funWithArg;
    void* arg;
    void* retval;
    struct thread *left;
    struct thread *right;
    char  *Stack;

}*curr_thread;

typedef struct thread thread;
int newCount,th_cntr,delCount, qinit,trun,tyield,tsuspend;

#ifdef __x86_64__
#define JB_SP 6
#define JB_PC 7

unsigned long encode_addr(unsigned long addr)
{
    unsigned long ret;
    asm volatile("xorq    %%fs:0x30,%0\n"
        "rolq    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
#define JB_SP 4
#define JB_PC 5

unsigned long encode_addr(unsigned long addr)
{
    unsigned long ret;
    asm volatile("xor    %%gs:0x18,%0\n"
        "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

struct queue{

    thread *front;
    thread *rear;
   
}ready_queue, suspended_queue, terminated_queue;

typedef struct queue queue;

struct timespec exec_time;

char errormsg = 'E';

void init_status(status *status,int tid)
{
    int sec = 0;
    status->id = tid ;
    status->state = READY;
    status->no_of_bursts = 0;
    status->total_exec_time = 0;
    status->avr_exec_time = 0;
    
    
}


void init_queue(queue *Q)
{
    Q->front = NULL;
    Q->rear = NULL;
}


void enqueue_ready(thread *th)
{
    queue *q = &(ready_queue);   
    if(q->front == NULL)
        q->front = q->rear = th;

    else
    {
        q->rear->right = th;
        th->left = q->rear;
        q->rear = th;
    }
}

void enqueue_susp(thread *th)
{
    queue *q = &(suspended_queue);   
    if(q->front == NULL)
        q->front = q->rear = th;

    else
    {
        q->rear->right = th;
        th->left = q->rear;
        q->rear = th;
    }
}

void enqueue_terminated(thread *th)
{
    queue *q = &(terminated_queue);   
    if(q->front == NULL)
        q->front = q->rear = th;

    else
    {
        q->rear->right = th;
        th->left = q->rear;
        q->rear = th;
    }
}

thread * dequeue_ready()
{

    thread *th = NULL;
    th = ready_queue.front;
    if(th == NULL)
        return NULL;
    ready_queue.front = ready_queue.front->right;
       
    if(ready_queue.front != NULL)
        (ready_queue.front)->left = NULL;
    else
        ready_queue.front = ready_queue.rear = NULL;
        th->right = NULL;       
    return th;


}

thread * dequeue_susp()
{

    thread *th = NULL;
    th = suspended_queue.front;
    if(th == NULL)
        return NULL;
    suspended_queue.front = suspended_queue.front->right;
       
    if(suspended_queue.front != NULL)
        (suspended_queue.front)->left = NULL;
    else
        suspended_queue.front = suspended_queue.rear = NULL;
        th->right = NULL;       
    return th;


}

thread * dequeue_terminated()
{

    thread *th = NULL;
    th = terminated_queue.front;
    if(th == NULL)
        return NULL;
    terminated_queue.front = terminated_queue.front->right;
       
    if(terminated_queue.front != NULL)
        (terminated_queue.front)->left = NULL;
    else
        terminated_queue.front = terminated_queue.rear = NULL;
        th->right = NULL;       
    return th;


}

int find_queue(int tid)
{
    thread *th;

    for(th=ready_queue.front; th ; th=th->right)
        if(th->tid == tid)
            return 0;

    for(th=suspended_queue.front; th ; th=th->right)
        if(th->tid == tid)
            return 1;
   

    for(th=terminated_queue.front; th ; th=th->right)
        if(th->tid == tid)
            return 2;
   
    return -1;
}

void *GetThreadResult (int tid)
{
    thread *t;
    if (find_queue(tid)==2)
        for(t=terminated_queue.front ; t != NULL ; t=t->right)
            if(tid == t->tid)
                return t->retval;
    else
        return &errormsg;
}       


int getID()
{
    return curr_thread->tid;   
}


void display_statistics(status *stat)
{
    int i=0;
    printf("\n\nStats for Thread : %d",stat->id);
    printf("\n");
    for(;i<25;printf("="),++i);
    printf("\n");
    printf("Current state : ");
    switch(stat->state)
    {
        case RUNNING :printf("RUNNING\n");break;
        case READY :printf("READY\n");break;
        case SUSPENDED :printf("SUSPENDED\n");break;
        default: printf("TERMINATED\n");
    }

    printf("Number of bursts : %d\n",stat->no_of_bursts);
    printf("Total execution time : %d msec\n",stat->total_exec_time);
    printf("Average execution time : %d msec\n\n",stat->avr_exec_time);
}


void clean()
{

    ualarm(0,0);
    

    thread *delThread;
    
    if(trun == true)
    {
        display_statistics(&(curr_thread->stat));
        free(curr_thread);
    }
    while ((delThread = dequeue_ready()) != NULL)
    {
        display_statistics(&(delThread->stat));
        free(delThread);
    }
    while ((delThread = dequeue_susp()) != NULL)
    {
        display_statistics(&(delThread->stat));
        free(delThread);
    }
   
    while ((delThread = dequeue_terminated()) != NULL)
    {
        display_statistics(&(delThread->stat));
        free(delThread);
    }

    printf("\n%d threads were created",newCount);
    printf("\n%d threads were deleted\n\nABORTING PROGRAM!!\n",delCount);
   
	

    exit(0);
}

void dispatch(int signum)
{
   
    int k;
    struct timespec curr_time;
    
    if(sigsetjmp(curr_thread->env,1) == 1)
        return;
       
    if(ready_queue.rear == NULL)
        if (trun == false)
            clean();
           
    if(trun == true)
    {
        curr_thread->stat.state=READY;
        enqueue_ready(curr_thread);
    }
       
	else if(tyield == true)
        {
            if(tsuspend == false)
            {
		curr_thread->stat.state = READY;
                tyield = false;
                enqueue_ready(curr_thread);
            }
            else
            {
                tsuspend = false;
                tyield = false;
                curr_thread->stat.state = SUSPENDED;
                enqueue_susp(curr_thread);

            }
        }

    clock_gettime(CLOCK_REALTIME, &curr_time);
    int nano_sec_diff = (curr_time.tv_nsec - exec_time.tv_nsec)/BILLION;
    int sec_diff = (curr_time.tv_sec - exec_time.tv_sec)*1000;
    
    curr_thread->stat.total_exec_time =  curr_thread->stat.total_exec_time + nano_sec_diff + sec_diff;
    curr_thread->stat.avr_exec_time = curr_thread->stat.total_exec_time/curr_thread->stat.no_of_bursts;
    
            curr_thread = dequeue_ready();
            
                curr_thread->stat.state=RUNNING;   
                clock_gettime(CLOCK_REALTIME, &exec_time);
                alarm(1);
                printf("\n");
                for(k=0;k<20;k++,printf("-"));
                  printf("\n");
                curr_thread->stat.no_of_bursts++;
                trun = true;
                siglongjmp(curr_thread->env,1);
            
}

void start ()
{
label: if(ready_queue.front == NULL)
	goto label;

    if(ready_queue.front != NULL)
    printf("\n\nExecution Begins\n\n");
    
   
    curr_thread = dequeue_ready();
    trun=true;

    signal(SIGALRM, dispatch);
    alarm(1);
   
    clock_gettime(CLOCK_REALTIME, &exec_time);
    curr_thread->stat.no_of_bursts++;

    siglongjmp(curr_thread->env, 1);
}

void wrapper()
{
    if(curr_thread->hasArg)
    {
        curr_thread->retval = (curr_thread->funWithArg)(curr_thread->arg);
    }
    else
    {
        (curr_thread->fun)();
    }

    delThread(curr_thread->tid);
}

int create(fun f)
{
    
    int rem_time=ualarm(0,0);
    thread *newThread;
    status stat;
   newThread = (thread *)malloc(sizeof(thread));
   
    if(newThread == NULL)
        return -1;
   
    newThread->tid = th_cntr++;
    init_status(&stat,newThread->tid);
     if(qinit == false)
    {   
        init_queue(&ready_queue);
        init_queue(&suspended_queue);
        init_queue(&terminated_queue);
        qinit = true;
    }

    sigsetjmp(newThread->env,1);

    newThread->Stack = (char*)malloc(Stack_Size);

       
    newCount++;

    unsigned long sp, pc;    
    pc = (unsigned long)wrapper;
    sp = (unsigned long)newThread->Stack + Stack_Size;
    newThread->env[0].__jmpbuf[JB_PC] = encode_addr(pc);
    newThread->env[0].__jmpbuf[JB_SP] = encode_addr(sp);
    
    
    newThread->hasArg = false;
    newThread->fun = f;
    newThread->stat = stat;
    newThread->left = newThread->right = NULL;
    newThread->retval = NULL;
    
    
    enqueue_ready(newThread);
    if(rem_time>0)
        ualarm(rem_time,0);
    return newThread->tid ;
}

int createWithArgs(funWithArg fptr, void *arg)
{
    int rem_time=ualarm(0,0);
    thread *newThread = (thread *)malloc(sizeof(thread));
    status status;

    if(newThread == NULL)
        return -1;
   
    newThread->tid = th_cntr++;
    init_status(&status,newThread->tid);

     if(qinit == false)
    {   
        init_queue(&ready_queue);
        init_queue(&suspended_queue);
        init_queue(&terminated_queue);
        qinit = true;
    }

    sigsetjmp(newThread->env,1);

    newThread->Stack = (char*)malloc(Stack_Size);
    newThread->hasArg = true;
    newThread->arg = arg;
    newThread->funWithArg = fptr;
    newThread->left = newThread->right = NULL;
    newCount++;
    
    unsigned long sp, pc;   
    pc = (unsigned long)wrapper;
    sp = (unsigned long)newThread->Stack + Stack_Size;
    newThread->env[0].__jmpbuf[JB_SP] = encode_addr(sp);
    newThread->env[0].__jmpbuf[JB_PC] = encode_addr(pc);

    newThread->stat = status;
    
       
    enqueue_ready(newThread);
    if(rem_time>0)
        ualarm(rem_time,0);
    return newThread->tid ;
}

int delThread(int tid)
{
    thread *th = NULL ;
    int q_id = find_queue(tid);
    queue *q = NULL;

    if(tid == curr_thread->tid)
    {
        trun = false;
        curr_thread->stat.state = TERMINATED;
        printf("DELETED Thread : %d\n",curr_thread->tid);
        enqueue_terminated(curr_thread);
        delCount++;
        dispatch(SIGALRM);
        return 0;
    }

    switch(q_id)
    {
        case 0: q = &(ready_queue);
		break;
	case 1: q = &(suspended_queue);
		break;
	case 2:printf("Thread %d is already deleted previously", tid);
            	return 0;
    }

    if((th = q->front) != NULL)
    {
    	for(; th!=NULL ; th=th->right)
    	{
              if(th->tid==tid)
               {
               		if (th->left)
         	               th->left->right = th->right;
				if (th == q->rear)
					q->rear = th->left;
                    	    if (th->right)
                        	th->right->left = th->left;
				if (th == q->front)
					q->front = th->right;
		    	    if ((th->right ==NULL) && (th->left == NULL))
		    		q->front =q->rear = NULL;
                    	    th->right = th->left = NULL;
			enqueue_terminated(th);
                    	return 0;
                } 
	}
    }	
            
    return -1;
}


void yield ()
{
    ualarm(0,0);
    trun = false;
    tyield = true;
    dispatch(SIGALRM);
}

void suspend(int tid)
{
    thread *th = NULL ;
    if(tid == curr_thread->tid)
    {
        tsuspend = true;
        curr_thread->stat.state = SUSPENDED;
        printf("SUSPENDED Thread : %d\n",curr_thread->tid);
        ualarm(0,0);
        trun = false;
        tyield = true;
        dispatch(SIGALRM);
    }

    else if(find_queue(tid) == 1)
        printf("Thread %d already in suspended state!!\n", tid);

    else
    {
        if((th=ready_queue.front) != NULL)
        {
            for(; th!=NULL ; th=th->right)
            {
                if(th->tid==tid)
                {
                     if (th->left)
                        th->left->right = th->right;
			if (th == ready_queue.rear)
				ready_queue.rear = th->left;
                    if (th->right)
                        th->right->left = th->left;
			if (th == ready_queue.front)
				ready_queue.front = th->right;
		    if ((th->right ==NULL) && (th->left == NULL))
		    	ready_queue.front = ready_queue.rear = NULL;
                    th->right = th->left = NULL;
                    break;
                }
            }
        }

        if(th==NULL)
            printf("Thread %d not in ready queue", tid);
        else
        {
            th->stat.state=SUSPENDED;
            printf("SUSPENDED Thread : %d\n",th->tid);
            enqueue_susp(th);
            return;
        }       
    }
   
}


void resume(int tid)
{
    thread *th = NULL;
   
    if(find_queue(tid) == 0)
        printf("Thread %d already in ready state!!\n", tid);
   
    else
    {
        if ((th=suspended_queue.front)!=NULL)
        {
            for(; th!=NULL ; th=th->right)
            {
                if(th->tid==tid)
                {
                    if (th->left)
                        th->left->right = th->right;
			if (th == suspended_queue.rear)
				suspended_queue.rear = th->left;
                    if (th->right)
                        th->right->left = th->left;
			if (th == suspended_queue.front)
				suspended_queue.front = th->right;
		    if ((th->right ==NULL) && (th->left == NULL))
		    	suspended_queue.front = suspended_queue.rear = NULL;
                    th->right = th->left = NULL;
                    break;
                }
            }
        }

        if(th==NULL)
            printf("Thread %d not in suspended queue", tid);

        else
        {
            th->stat.state=READY;
            printf("RESUMED Thread : %d\n",th->tid);
            enqueue_ready(th);
        }
       
    }
}

status * GetStatus (int tid)
{
    thread *th;
    status * stat = NULL;
    queue *q = NULL;
    int q_id = find_queue(tid);

    switch(q_id)
    {

	case 0: q = &(ready_queue);
		break;
   	case 1: q = &(suspended_queue);
		break;
   	case 2: q = &(terminated_queue);
		break;
	default: printf("Thread %d never existed!!", tid);
		return NULL;
    }
   
    for(th=q->front; th != NULL ; th=th->right)
    {
        if(th->tid == tid)
        {
            *(stat) = th->stat;
            return stat;
        }   
    }
}


//include library here
