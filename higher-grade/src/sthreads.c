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
			if(cursor -> state == ready){
				cursor -> state = running;
				swapcontext(&sched_ctx, &(cursor -> ctx));
			}
		}
	}
}

void 
init_context (ucontext_t *ctx, ucontext_t *next)
{
	void * stack = malloc(STACK_SIZE);

	if(stack == NULL){
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
	timer.it_value.tv_usec = ms*10;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	if(setitimer (ITIMER_REAL, &timer, NULL) < 0){
		perror("timer set");
		exit(EXIT_FAILURE);
	}
}

void
timer_handler (int signum)
{
	printf("------------Timer Inturrupt!!----------\n");
	set_timer(timer_handler, MS);
	yield();
}

/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/
int init(){
	//init sched_context
	init_context(&sched_ctx,NULL);
	makecontext(&sched_ctx,scheduler, 0);

	//allocate table
	table = (thread_t*) malloc (sizeof(thread_t));
	if(table == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	//init main_thread
	thread_t* main_thread;
	main_thread = (thread_t*) malloc(sizeof(thread_t));
	if(main_thread == NULL){
		perror("malloc main thread");
		exit(EXIT_FAILURE);
	}

	ucontext_t main_ctx;	
	init_context(&main_ctx, NULL);

	main_thread->ctx = main_ctx;
	main_thread->tid = ++tidnum;
	main_thread->state = ready; 
	main_thread->next = 0x0;
	table -> next = main_thread;
	tail = main_thread;

	//init main context
	swapcontext(&main_ctx,&sched_ctx);
	set_timer(timer_handler,MS);
	return 1;
}

tid_t 
spawn(void (*start)())
{
	thread_t* new_thread;
	new_thread = (thread_t*) malloc(sizeof(thread_t));
	if(new_thread == NULL){
		perror("spawn malloc error");
		exit(EXIT_FAILURE);
	}

	ucontext_t new_ctx;
	init_context(&new_ctx, NULL);
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
yield()
{
	thread_t *cur_thread = cursor;
	if(cur_thread -> state != running){
		perror("state error");
		exit(EXIT_FAILURE);
	}

	cur_thread -> state = ready;
	swapcontext(&(cur_thread -> ctx), &sched_ctx);
}

void
done()
{
	thread_t *cur_thread = cursor;
	cursor -> state = terminated;

	for(thread_t * itr = table -> next; itr != 0x0; itr = itr -> next){
		if(itr -> state == waiting){
			itr -> state = ready;
		}
	}
	swapcontext(&(cur_thread -> ctx), &sched_ctx);
}

tid_t 
join() 
{
	thread_t *cur_thread = cursor;
	cursor -> state = waiting;
	swapcontext(&(cur_thread -> ctx), &sched_ctx);

	tid_t tid;
	for(thread_t * itr = table; itr != 0x0; itr = itr -> next){
		if(itr -> next != 0x0 && itr -> next -> state == terminated){
			//erase this thread from list
			thread_t * gone = itr -> next;
			tid = gone -> tid;

			itr -> next = gone -> next;
			if(tail == gone){
				tail = itr;
			}
			free(gone);
		}
	}
	return tid;
}
