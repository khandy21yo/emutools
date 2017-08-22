//
// Yes, you can write gosub/return in c!
//
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

//
// These macros allocate the names SETUP, GOSUB, RETURN, stack and stackp.
// They are done as macros instead of inline code to make it easier to understand.
// Note the hardcoded limit of 20 levels of GOSUB.
// Also note that there is no error checking of stack boundries.
//
#define SETUP static jmp_buf stack[20]; static int stackp=0;
#define GOSUB(x) if (setjmp(stack[stackp++]) == 0) { goto x; }
#define RETURN longjmp(stack[--stackp], 47);


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
