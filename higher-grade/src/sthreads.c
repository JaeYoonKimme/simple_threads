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

#include "sthreads.h"

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100

/*******************************************************************************
                             Global data structures

                Add data structures to manage the threads here.
********************************************************************************/
thread_t ** table;
int tidnum = 0;

ucontext_t sched_ctx;
/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/
void 
scheduler()
{
	thread_t * t;
	while(1){	
		for(int i = 0; i < tidnum; i++){
			t = table[i];
			printf("tid : %d\n",t -> tid);
			swapcontext(&sched_ctx, &(t -> ctx));
		}
	}
}



/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/

void init_context(ucontext_t *ctx, ucontext_t *next){
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

int init(){
	//init sched_context
	init_context(&sched_ctx,NULL);
	makecontext(&sched_ctx,scheduler, 0);

	//allocate table
	table = (thread_t**) malloc (sizeof(thread_t*));
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
	main_thread->tid = tidnum;
	main_thread->state = ready; //?not sure
	table[tidnum] = main_thread;
	tidnum ++;

	//init main context
	swapcontext(&main_ctx,&sched_ctx);

	return 1;
}

tid_t spawn(void (*start)()){
	
	return -1;
}

void yield(){
	//여기서 콘택스트를 하나 만들고,
	//스케쥴러와 스왑해야한다
	//state 를 언제 어떻게 바꿔줘야하는지 ..
}

void  done(){
}

tid_t join() {
  return -1;
}
