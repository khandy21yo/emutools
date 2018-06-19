//
// Yes, you can write gosub/return in c!
//
#include <stdio.h>
#include <stdlib.h>

//
// Here is a version for GCC only of the GOSUB/RETURN from
// http://www.di.unipi.it/~nids/docs/gosub_greturn_with_gcc.html
// and slightly modified to match the setjmp version slightly better.
//
#include <stdlib.h>

typedef struct __basic_rp_stack {
  void *rp;
  struct __basic_rp_stack *next;
} __basic_rp_stack_t;

/* per thread basic return stack */
__thread __basic_rp_stack_t *__rps = NULL;
#define SETUP

void __rps_push( void *r ) {
  __basic_rp_stack_t *x = malloc( sizeof(__basic_rp_stack_t) );
  if(!x) abort();
  else {
    x->next = __rps;
    x->rp = r;
    __rps = x;
  }
}

void* __rps_pop( ) {
  if( !__rps ) abort();
  else {
    __basic_rp_stack_t *x = __rps;
    void *r = x->rp;
    __rps = x->next;
    free( x );
    return r;
  }
}

#define __C(a, b) a ## b
#define __U(p, q) __C(p, q)

#define GOSUB( LABEL ) do { __rps_push( && __U( __rp, __LINE__ ) ); goto LABEL ; __U( __rp, __LINE__ ) : ; } while(0)
#define RETURN do { void *__rp = __rps_pop( ); goto *__rp; } while(0)


//
// Ok, do the dirty deed
//
int main()
{
	//
	// Initialization. Could also be done globally.
	//
	SETUP;

	//
	// Use it a couple of times
	//
	GOSUB(x1);
	GOSUB(x1);

	exit(0);

	//
	// First subroutine
	//
x1:
	printf("here\n");
	GOSUB(x2);
	printf("back\n");
	RETURN;

	//
	// 2nd subroutine.
	// Yup, you can use multiple levels
	//
x2:
	printf("there\n");
	RETURN;
}
