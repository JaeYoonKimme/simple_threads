/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
   following define.
*/
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
   flag must also be used to suppress compiler warnings.
*/

#include <signal.h>   /* SIGSTKSZ (default stack size), MINDIGSTKSZ (minimal
                         stack size) */
#include <stdio.h>    /* puts(), printf(), fprintf(), perror(), setvbuf(), _IOLBF,
                         stdout, stderr */
#include <stdlib.h>   /* exit(), EXIT_SUCCESS, EXIT_FAILURE, malloc(), free() */
#include <ucontext.h> /* ucontext_t, getcontext(), makecontext(),
                         setcontext(), swapcontext() */
#include <stdbool.h>  /* true, false */

#include <sys/time.h>
#include <signal.h>
#include <string.h>

#define MS 50
#include "sthreads.h"

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100

/*******************************************************************************
                             Global data structures

                Add data structures to manage the threads here.
********************************************************************************/
thread_t * table;
thread_t * tail;
thread_t * cursor;
int tidnum = 0;

/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/
ucontext_t sched_ctx;

void 
scheduler ()
{
	while(1){	
		for(cursor = table -> next; cursor != 0x0; cursor = cursor -> next){	
			if(cursor != 0x0 && cursor -> state == ready){
				cursor -> state = running;
				set_timer(timer_handler, MS);
				if(swapcontext(&sched_ctx, &(cursor -> ctx)) < 0){
					perror("swap_sched");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
}

void 
init_context (ucontext_t *ctx, ucontext_t *next)
{
	void * stack = malloc(STACK_SIZE);

	if(stack == 0x0){
		perror("STACK MALLOC");
		exit(EXIT_FAILURE);
	}

	if(getcontext(ctx) < 0){
		perror("GETCONTEXT");
		exit(EXIT_FAILURE);
	}

	ctx->uc_link = next;
	ctx->uc_stack.ss_sp = stack;
	ctx->uc_stack.ss_size = STACK_SIZE;
	ctx->uc_stack.ss_flags = 0;
}

void 
set_timer (void (*handler) (int), int ms)
{
	struct itimerval timer;
	signal(SIGALRM, handler);

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 100;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	if(setitimer (ITIMER_REAL, &timer, 0x0) < 0){
		perror("timer set");
		exit(EXIT_FAILURE);
	}
}
	
long
disable_timer ()
{
	struct itimerval timer;
	struct itimerval old;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	if(setitimer(ITIMER_REAL, &timer, &old) < 0){
		perror("disable timer set");
		exit(EXIT_FAILURE);
	}

	return old.it_value.tv_usec;
}

void
timer_handler (int signum)
{
	printf("------------Timer Inturrupt!!----------\n");
	//set_timer(timer_handler, MS);
	yield();
}

/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/
int init (){
	//init sched_context
	init_context(&sched_ctx, 0x0);
	makecontext(&sched_ctx,scheduler, 0);

	//allocate table
	table = (thread_t*) malloc (sizeof(thread_t));
	if(table == 0x0){
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	//init main_thread
	thread_t* main_thread;
	main_thread = (thread_t*) malloc(sizeof(thread_t));
	if(main_thread == 0x0){
		perror("malloc main thread");
		exit(EXIT_FAILURE);
	}

	ucontext_t main_ctx;	
	init_context(&main_ctx, 0);

	main_thread->ctx = main_ctx;
	main_thread->tid = ++tidnum;
	main_thread->state = ready; 
	main_thread->next = 0x0;
	table -> next = main_thread;
	tail = main_thread;

	//init main context
	if(swapcontext(&(main_thread -> ctx), &sched_ctx) < 0){
		perror("swap init");
		exit(EXIT_FAILURE);
	}
	set_timer(timer_handler,MS);
	return 1;
}

tid_t 
spawn (void (*start)())
{
	thread_t* new_thread;
	new_thread = (thread_t*) malloc(sizeof(thread_t));
	if(new_thread == 0x0){
		perror("spawn malloc error");
		exit(EXIT_FAILURE);
	}

	ucontext_t new_ctx;
	init_context(&new_ctx, &sched_ctx);
	makecontext(&new_ctx, start, 0);
	new_thread->ctx = new_ctx;
	new_thread->tid = ++tidnum;
	new_thread->state = ready;
	new_thread->next = 0x0;
	tail -> next = new_thread;
	tail = new_thread;

	return new_thread -> tid;
}

void 
yield ()
{
	disable_timer();
	thread_t *cur_thread = cursor;
	if(cur_thread -> state != running){
		if(cur_thread -> state == terminated){
			printf("termin...\n");
		}
		if(cur_thread -> state == ready){
			printf("ready...\n");
		}
		perror("state error");
		exit(EXIT_FAILURE);
	}

	cur_thread -> state = ready;

	if(swapcontext(&(cur_thread -> ctx), &sched_ctx) < 0){
		perror("swap yield");
		exit(EXIT_FAILURE);
	}
}

void
done ()
{
	disable_timer();
	thread_t *cur_thread = cursor;

	while(1){
		int found = 0;
		for(thread_t *itr = table -> next; itr != 0x0; itr = itr -> next){
			if(itr -> state == waiting){
				itr -> state = ready;
				found = 1;
			}
		}
		if(found == 0){
			yield();
		}
		else{
			break;
		}
	}
	
	cur_thread -> state = terminated;
	if(swapcontext(&(cur_thread -> ctx), &sched_ctx) < 0){
		perror("swap done");
		exit(EXIT_FAILURE);
	}
}

tid_t 
join () 
{
	disable_timer();
	thread_t *cur_thread = cursor;
	cursor -> state = waiting;
	if(swapcontext(&(cur_thread -> ctx), &sched_ctx) < 0){
		perror("swap join");
		exit(EXIT_FAILURE);
	}
	disable_timer();
	tid_t tid = -1;
	for(thread_t * itr = table; itr != 0x0; itr = itr -> next){
		if(itr -> next != 0x0 && itr -> next -> state == terminated){
			thread_t * gone = itr -> next;
			tid = gone -> tid;

			itr -> next = gone -> next;
			if(tail == gone){
				tail = itr;
			}
			free(gone -> ctx.uc_stack.ss_sp);
			free(gone);
			break;
		}
	}
	set_timer(timer_handler, MS);
	return tid;
}

/*******************************************************************************
                    Implementation of thread MUTEX LOCK API
********************************************************************************/
void
mutex_init (mutex_t* mutex)
{
	//0 -> available 
	//1 -> held
	mutex -> flag = 0;
}

void
mutex_lock (mutex_t* mutex)
{	
	long old = disable_timer();	
	while(mutex -> flag == 1){
		//set_timer(timer_handler, old/1000);
		yield();
	}

	mutex -> flag = 1;
	set_timer(timer_handler, old/1000);
}

void
mutex_unlock (mutex_t* mutex)
{
	long old = disable_timer();
	mutex -> flag = 0;
	set_timer(timer_handler, old/1000);
}


void
cond_wait (cond_t * cond, mutex_t * mutex)
{

}






