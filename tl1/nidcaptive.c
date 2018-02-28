/** \file nidcaptive.c
 * \brief TL1 shell
 *
 * \author 6/28/2008 crp Initial version
 * \author 01/26/2009 kth C version
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

static char *versionno = "V1.0.0";

/*
 * Global variables
 */
char *exename;			/**< Name of executable */
char *filename;			/**< Name of command file */
char hostname[HOST_NAME_MAX];	/**< Name of host */
int echocommands = FALSE;	/**< Echo commands before executing them? */
int auth = 0;			/**< Authorized flag */

/**\brief Usage message
 *
 * Displays usage parameters.
 */
static void usage(void)
{
	fprintf(stderr,
		"Usage: %s [-v] [-c command] [-h] [-x] [filename]\n",
		exename);
}

/**\brief Version message
 *
 * Displays the version number
 */
static void version(void)
{
	fprintf(stderr,
		"Version %s %s\n",
		exename, versionno);
}

/** /brief Captive error
 *
 * This is an interrupt routine that prints out a usage
 * message, and resets everything.
 */
void captive(void)
{
//	inidhost();
	fprintf(stderr, "\nConsole escape. Commands are:\n"
		" e      exit telnet\n"
		"%s>>", hostname);
	// return to interrupt point
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
static int tl1parsestream(FILE *tl1stream)
{
	char buffer[256];

	/*
	 * Determine which read routine to use.
	 * If we are attached to a terminal, use the readline version,
	 * else try to read as a simple stream.
	 */
	if (isatty(fileno(tl1stream)))
	{
#if 0
		printf("TTY Stream\n");
#endif
		tl1_rl_readline(tl1stream);
	}
	else
	{
#if 0
		printf("Not a tty stream\n");
#endif
		/*
		 * Read entire file, one line at a time.
		 */
		while(fgets(buffer, sizeof(buffer), tl1stream) != NULL)
		{
			/*
			 * Strip off terminator (chomp)
			 */
			int len = strlen(buffer);
			if (buffer[len-1] == '\n')
			{
				buffer[len-1] = '\0';
			}

			/*
			 * Process line
			 */
			tl1_parse_line(buffer);
		}
	}

	return 0;
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
static int tl1parsefile(
	const char *filename		/**< Name of file to parse */
)
{
	FILE *fs;		/* Input file stream */
	int status;

	/*
	 * Open input file
	 */
	fs = fopen(filename, "r");
	if (fs == 0)
	{
		/* \FIXME: Need error code */
		return -1;
	}

	/*
	 * Parse all data from file
	 */
	status = tl1parsestream(fs);

	/*
	 * Clean up
	 */
	fclose(fs);
	return status;
}


/** \brief main function
 */
int main(
	int argc,		/**< Number of arguments */
	char *argv[]		/**< Array of arguments */
)
{
	int opt;		/* Used for processing command line options */
	int status;		/* Status returned from various calls */
	char *command = NULL;	/* command from command line option '-c' */

fprintf(stderr, "***TEST LOGIN***: %s\n", "ACT-USER:INID0000001:console:123:bspnlk54tzToxScg;");

	/*
	 * Process command line arguments
	 */
	exename = argv[0];
	if (gethostname(hostname, sizeof(hostname) - 1) != 0)
	{
		fprintf(stderr, "Unable to determine host name\n");
		strcat(hostname, "Unknown");
	}

	/*
	 * Parse command line arguments
	 */
	while ((opt = getopt(argc, argv, "vhxc:")) != -1)
	{
		switch(opt)
		{
		case 'v':
			version();
			exit(EXIT_SUCCESS);

		case 'h':
		case '?':
			usage();
			exit(EXIT_SUCCESS);

		case 'x':
			echocommands = TRUE;
			break;

		case 'c':
			command = strdup(optarg);
			break;

		default:
			exit(EXIT_FAILURE);
		}
	}

	/*
	 * If we were passed a file name, use that, otherwise
	 * use standard input
	 */
	if (command != NULL)
	{
		status = tl1_command(command);
		free(command);
		return status;
	}

	if (optind >= argc)
	{
		status = tl1parsestream(stdin);
		if (status)
		{
			tl1_error(status, stderr, 0, NULL);
		}
	}
	else
	{
		status = tl1parsefile(argv[optind]);
		if (status)
		{
			tl1_error(status, stderr, 0, NULL);
		}
	}

	return EXIT_SUCCESS;
}

