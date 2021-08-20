#include <stdlib.h>   // exit(), EXIT_FAILURE, EXIT_SUCCESS
#include <stdio.h>    // printf(), fprintf(), stdout, stderr, perror(), _IOLBF
#include <stdbool.h>  // true, false
#include <limits.h>   // INT_MAX

#include "sthreads.h" // init(), spawn(), yield(), done()

/*******************************************************************************
                   Functions to be used together with spawn()

    You may add your own functions or change these functions to your liking.
********************************************************************************/

/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
 */
void numbers() {
  int n = 0;
  while (true) {
    printf(" n = %d\n", n);
    n = (n + 1) % (INT_MAX);
    if (n > 3) done();
    yield();
  }
}

/* Prints the sequence a, b, c, ..., z over and over again.
 */
void letters() {
  char c = 'a';

  while (true) {
      printf(" c = %c\n", c);
      if (c == 'f') done();
      yield();
      c = (c == 'z') ? 'a' : c + 1;
    }
}

/* Calculates the nth Fibonacci number using recursion.
 */
int fib(int n) {
  switch (n) {
  case 0:
    return 0;
  case 1:
    return 1;
  default:
    return fib(n-1) + fib(n-2);
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This is deliberately an unnecessary slow and CPU intensive
   implementation where each number in the sequence is calculated recursively
   from scratch.
*/

void fibonacci_slow() {
  int n = 0;
  int f;
  while (true) {
    f = fib(n);
    if (f < 0) {
      // Restart on overflow.
      n = 0;
    }
    printf(" fib(%02d) = %d\n", n, fib(n));
    n = (n + 1) % INT_MAX;
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This implementation is much faster than fibonacci().
*/
void fibonacci_fast() {
  int a = 0;
  int b = 1;
  int n = 0;
  int next = a + b;

  while(true) {
    printf(" fib(%02d) = %d\n", n, a);
    next = a + b;
    a = b;
    b = next;
    n++;
    if (a < 0) {
      // Restart on overflow.
      a = 0;
      b = 1;
      n = 0;
    }
  }
}

/* Prints the sequence of magic constants over and over again.

   https://en.wikipedia.org/wiki/Magic_square
*/
void magic_numbers() {
  int n = 3;
  int m;
  while (true) {
    m = (n*(n*n+1)/2);
    if (m > 0) {
      printf(" magic(%d) = %d\n", n, m);
      n = (n+1) % INT_MAX;
    } else {
      // Start over when m overflows.
      n = 3;
    }
    yield();
  }
}

int id = 0;
int count = 0;
void counter() {
	//int count = 0;
	int own_id = id ++;
	
	for(int i = 0; i < 100000; i++){
		printf("id : %d -> count : %d\n",own_id,i);
		count ++;
		
		if(i == 100000){
			done();
		}
	}
}

/*******************************************************************************
                                     main()

            Here you should add code to test the Simple Threads API.
********************************************************************************/
void test_senario1(){
	puts("\n==== Test program for the Simple Threads API ====\n");
	init(); // Initialization
	spawn(counter);
	printf("spawned counter1\n");
	spawn(counter);
	printf("Spawned counter2\n");
	spawn(counter);
	printf("spawned counter3\n");
	spawn(counter);
	printf("spawned counter4\n");
		//while(true){
	int one = join();
	int two = join();
	int three = join();
	int four = join();
	//yield();
	//join();	
	//while(1){
	//}
	printf("All done %d %d %d %d\n", one, two, three, four);


}

mutex_t m;


void
worker()
{
	//int own_id = id++;
	mutex_lock(&m);
	for(int i = 0; i < 1000000; i ++){
		//printf("%d -> %d\n",own_id,i);
		int tmp = count;
		tmp ++;
		count = tmp;
	}
	mutex_unlock(&m);
	done();
}


int chkr = 0;
mutex_t mutex;
void
test1()
{
  while(1){
    printf("1 locking\n");
    mutex_lock(&mutex);
    // cond_wait(&cond, &mutex);
    printf("========here %d\n", chkr++);
    mutex_unlock(&mutex);
    printf("1 unlocking\n");
  }
}
void
test2()
{
  while(1){
    printf("2 locking\n");
    mutex_lock(&mutex);
    // cond_signal(&cond);
    printf("-------there %d\n", chkr++);
    mutex_unlock(&mutex);
    printf("2 unlocking\n");
  }
}


int main(){
	puts("\n==== Test program for the Simple Threads API ====\n");
	init(); // Initialization
	spawn(test1);
	spawn(test2);


	join();
	join();
	//printf("alldone. result count : %d\n",count);
}
