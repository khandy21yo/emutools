/** \file tl1readline.c
 * \brief tl1 interface to readline library
 *
 * This module contains the terminal interface code to read
 * commands from a terminal using the readline libray.
 *
 * \author Kevin Handy 02/20/2009
 */
#define TEST_DRIVER

/*
 * Include headers
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>

#include "tl1parse.h"

/*
 * Local variables
 */
static char *fullline = NULL;

/** Force input to upper case
 *
 * Convert user input to upper case.
 * The tl1 commands are case insensitive,
 * and forcing them to upper case makes it
 * easier to fo matching.
 *
 * This function is inserted into the
 * rl_getc_function callback.
 */
#if 0
static int tl1_rl_upper(FILE* stream)
{
	int ch;
	ch = rl_getc(stream);
	ch = _rl_to_upper(ch);
	return ch;
}
#endif


/** Handle semicolons
 *
 * When the user types a semicolon,
 * attach it to the end of the input line,
 * display the added semicolon,
 * do a newline,
 * flag the input as being completed.
 */
static int tl1_rl_semicolon(int count, int key)
{
	rl_end_of_line(1, 0);
	rl_insert(count, key);
	if (rl_redisplay_function)
	{
		(*rl_redisplay_function)();
	}
	rl_done = 1;
	if (rl_outstream)
	{
		fprintf(rl_outstream, "\n");	// \FIXME: Is there a better way?
	}
	else
	{
		printf("\n");			// \FIXME: Is there a better way?
	}
	return 0;
}

/** \brief Handler for terminal interrupts
 *
 * Handles terminl interruptions, usually a
 * control-C from the user.
 */
static void handle_sigint(int sig)
{
	/*
	 * Cancel any partial commands left over
	 */
	if (fullline)
	{
		free(fullline);
		fullline = NULL;
	}

	printf("^C\n");
}

/** \brief Read commands from a terminal
 *
 * Uses the readline functions to get commands from
 * a terminal.
 */
int tl1_rl_readline(FILE *stream)
{
	char *buffer = NULL;

	/*
	 * Create signal handlers
	 */
	signal(SIGINT, handle_sigint);		// Control-C
	signal(SIGTSTP, handle_sigint);		// Control-Z

	/*
	 * Configure readline
	 */
//	rl_getc_function = tl1_rl_upper;
//	rl_variable_bind("horizontal-scroll-mode", "on");

//	rl_bind_key('\n', rl_insert);
//	rl_bind_key('\r', rl_insert);
	rl_bind_key('\t', rl_insert);
	rl_bind_key(';', tl1_rl_semicolon);

	/*
	 * Endless cycle
	 */
	while (1)
	{
		char *prompt;

		if (fullline && *fullline)
		{
			prompt = "> ";
		}
		else
		{
			prompt = "tl1> ";
		}

		/*
		 * Grab a line of user entry
		 */
		buffer = readline(prompt);

#ifdef SPECIAL_OPTIONS
		/*
		 * Handle special commands.
		 *
		 * These are special commands that allow breaking out of the
		 * shell and bypasses security.
		 *
		 * They must start/end on a single line.
		 */
		if (buffer && *buffer && fullline == NULL)
		{
			/*
			 * Exit
			 */
			if (strcasecmp("e", buffer) == 0)
			{
				/*
				 * Crash out
				 */
				free(buffer);
				exit(0);
			}

			/*
			 * Shell
			 */
			if (strcasecmp("sh", buffer) == 0)
			{
				free(buffer);
				system("/bin/sh");
				continue;
			}

		}
#endif

		/*
		 * Add to current command line.
		 * Make sure we don't mung 'buffer',
		 * since we need it again later for history.
		 */
		if (buffer && *buffer)
		{
			int len = strlen(buffer);
			char *newbuffer;

			/*
			 * If we have a current command being created,
			 * merge in the new text, otherwise just use
			 * the new data.
			 */
			if (fullline)
			{
				newbuffer = malloc(strlen(fullline) + len + 1);
				strcpy(newbuffer, fullline);
				free(fullline);
			}
			else
			{
				newbuffer = malloc(len + 1);
				newbuffer[0] = '\0';
			}
			strcat(newbuffer, buffer);

			fullline = newbuffer;

			/*
			 * If the command is terminated with a ';',
			 * Then do something with it.
			 */
			len = strlen(fullline);
			if (fullline[len - 1] == ';')
			{
				tl1_parse_line(fullline);
				free(fullline);
				fullline = NULL;
			}

		}

		/*
		 * Add to command line history
		 * Lose any semi-colons at end of lines
		 * because they are terminators.
		 */
		if (buffer && *buffer && *buffer != ';')
		{
			int len = strlen(buffer);

			if (buffer[len - 1] == ';')
			{
				buffer[len - 1] = '\0';
				add_history(buffer);
			}
			add_history(buffer);
		}

		/*
		 * Clean up afterwards
		 */
		if (buffer)
		{
			free(buffer);
		}
	}
}
