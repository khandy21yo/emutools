/** \file tl1parse.c
 * \brief parse TL1 commands
 *
 * 7/24/2008 crp Initial version
 */

/*
 * Include headers
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "tl1parse.h"

/*
 * Global variables
 */
char *exename;			/**< Name of executable */
char *filename;			/**< Name of command file */
char hostname[HOST_NAME_MAX];	/**< Name of host */
int echocommands = FALSE;	/**< Echo commands before executing them? */

/**\brief Usage message
 */
static void usage(void)
{
	fprintf(stderr, "Usage: %s filename\n", exename);
}


/** \brief main function
 */
int main(
	int argc,		/**< Number of arguments */
	char *argv[]		/**< Array of arguments */
)
{
	int opt;

	exename = argv[0];
	if (gethostname(hostname, sizeof(hostname) - 1) != 0)
	{
		fprintf(stderr, "Unable to determine host name\n");
		strcat(hostname, "Unknown");
	}

	/*
	 * Parse command line arguments
	 */
	while ((opt = getopt(argc, argv, "hx")) != -1)
	{
		switch(opt)
		{
		case 'h':
			usage();
			exit(EXIT_SUCCESS);

		case 'x':
			echocommands = TRUE;
			break;

		default:
			exit(EXIT_FAILURE);
		}
	}

	/*
	 * If we were passed a file name, use that, otherwise
	 * use standard input
	 */
	if (optind >= argc)
	{
		tl1parsestream(stdin);
	}
	else
	{
		tl1parsefile(argv[optind]);
	}
}

/** \brief Process commands from a file name
 *
 * When given a file name,
 * try to open it and process all the commands held therin.
 *
 * This function should be recursive.
 *
 * \returns Status value
 * . 0 Success
 * . non zero is a failure
 */
int tl1parsefile(
	const char *filename		/**< Name of file to parse */
)
{
}

/** \brief Process commands from an open stream
 *
 * When given an already open stream,
 * try to process all the commands available from that stream.
 *
 * This function should be recursive.
 *
 * \returns Status value
 * . 0 Success
 * . non zero is a failure
 */
int tl1parsestream(FILE *tl1stream)
{
}

#if 0
COMMAND=""

while read INPUT;
do
	TEST=`echo $INPUT | sed 's/.*;$/XXX/'`
	if [ x"${TEST}" = x"XXX" ]; then
		INPUT=`echo $INPUT | sed 's/	//g' | sed 's/: /:/g'`
		COMMAND="$COMMAND$INPUT"
		set `echo $COMMAND | sed 's/\ //g' | sed 's/;$//g' | sed 's/:/ /g'`
		CMD=`echo $1 | tr '[A-Z]' '[a-z]'`
		if [ -x /bin/tl1/${CMD} ]; then
			case "${CMD}" in
				${CMD}) /bin/tl1/${CMD} "${COMMAND}"
				exit_status=$?
				;;
			esac
		else
			echo ""
			echo -n "   ${HOSTNAME} ";date
			echo "M  1 DENY"
			echo "   IICM"
			echo "   /* Input, Invalid VERB */"
		fi
		COMMAND=""
	else
		COMMAND="$COMMAND$INPUT"
	fi

done < "$FILENAME"

exit 0
#endif
