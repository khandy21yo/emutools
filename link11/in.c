/* Input functions */

# include "link.h"

/* define maximum size of checksum contents */
# define	MAXSIZE		400

static int getb();
static int read_mod();
static void inerror(char *mess);


static char	*Fname = NULL;	/* name of current input file */
static FILE	*Fp = NULL;	/* file pointer of current file */
static char	Buff[MAXSIZE];	/* buffer for current checksum module */
static char	*Next;		/* next byte to be popped from buffer */
static int	Count;		/* number of bytes left */
static int	Type;		/* type of checksum module */
static char	No_code = 0;	/* flag set if a code section was attempted to
				** be found but was not there */


/***************************  ch_input  ************************************/


void ch_input(	/* change input checksum buffer contents */
	char	*newfile,
	int	newmod)

{
	if (Fname == NULL || strcmp(Fname, newfile))	/* new file is
							** different */
	{
		Fname = newfile;
		if (Fp != NULL)
			fclose(Fp);
		if ((Fp = fopen(Fname, "r")) == NULL)
			inerror("not found");
		Type = 0;

//		getb();	// kth: RSX object format
//		getb();
	}
	if (newmod != Type)			/* if not right module type already */
		while (newmod != read_mod())	/* read until correct module type */
		{
			/* check for missing code section */
			if ( newmod == CODE && Type == SYMBOLS)
			{
				No_code = 1;
				break;
			}
			if (Type == 6)		/* check for EOF module */
			{
				No_code = 1;
				break;
			}
				/*
				inerror("EOF \(linker error\)");
				*/
		}
}


/**************************  morebytes  ************************************/


int morebytes()	/* returns 1 if there are unread bytes of the current */
		/* checksum module type, returns 0 if not */
{
	int	temptype;

	if (No_code)	/* if no code section, return 0 and reset */
	{
		No_code = 0;
		return (0);
	}
	else if (Count > 0)
		return (1);
	else
	{
			/* read next module and check for same type */
		temptype = Type;
		if (temptype == read_mod())
			return (1);
		else
			return (0);
	}
}


/******************************  getbyte  ************************************/


int getbyte()	/* return next byte of current checksum module type */

{
		/* check for empty buffer, if so check if next module is */
		/* the same type */
	if ((Count == 0) && !morebytes())
	{
		lerror("End of checksum module");
		return 0; /* NOTREACHED */
	}
	else
	{
		Count--;
		return (*Next++ & 0377);
	}
}


/****************************  getword  ************************************/


WORD getword()	/* return next word */
{
	int	temp;

	temp = 0377 & getbyte();
	return (0400 * getbyte() + temp);
}


/****************************  inerror  **********************************/


static void inerror(mess)	/* print error message and filename then exit. */
		/* called when a user error has occurred concerning the */
		/* input file */
char 	*mess;
{
	fprintf(stderr, "%s: %s\n", Fname, mess);
	bail_out();
}


/****************************************************************************/


int	sum;	/* sum of input bytes */


/***************************  read_mod  **************************************/


static int read_mod()	/* read a checksum module and return type */

{
	int	i;
	int	c1, c2;

	sum = 0;
	if ((c1 = getb()) != 1)
	{
//		if (feof(Fp) && No_code)
		if (No_code)
			return ( 6 );
		else
		{
			// Try to see if we are at eof.
			// Unlike Unix, DEC OS's bad disk blocks to 512.
			// I'm assuming to nulls at this point mean EOF.
			//
			c2 = getb();
			if ((c1 == 0 || c1 == -1) && (c2 == 0 || c2 == -1))
			{
				No_code = 1;
				return ( 6 );
			}
			inerror("Not in object file format");
		}
	}
			/* clear zero byte */
	getb();
			/* Count = next word - 6 (# of bytes in header) */
	Count = getb();
	Count += 0400 * getb() - 6;
	if (Count > MAXSIZE)
	{
//		printf("DEBUG: Count = %d\n", Count);
		lerror("checksum size too large");
	}
	Type = getb();
			/* clear zero byte */
//	printf("DEBUG: Count = %d, Type = %d\n", Count, Type);
	getb();
			/* read checksum contents into buffer */
	for (i = 0; i < Count; i++)
		Buff[i] = getb();
			/* read checksum */
	getb();
	if (sum % 0400 != 0)
	{
//		printf("DEBUG: sum = %d\n", sum);
		inerror("Checksum error, reassemble");
	}
			/* clear zero trailer byte if Count even */
//	if (Count % 2 == 0)
//		getb();
			/* set pointer to next character to be read */
	Next = Buff;
	return (Type);
}


/*************************  getb  ***************************************/


static int getb()		/* get a byte from input file, add to "sum" */
		/* check for EOF, return the byte */
{
	int	k;

	sum += (k = getc(Fp));

// printf("getb: char %d, sum %d\n", k, sum);
	if (k == EOF)
		No_code = 1;
	return (0377 & k);
}
