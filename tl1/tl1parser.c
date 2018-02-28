/** \file tl1parser.c
 * \brief Parse tl1 command lines
 *
 * \author Kevin Handy 2/26/2009
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "tl1parse.h"

/*
 * Local variables
 */
static char *command = NULL;		/**< Command line being built up */

/** \brief Parse (partial) command line.
 *
 * Parses a users input.
 * Trims leading/trailing spaces from the command line.
 * Breaks up the users input between semicolons and
 * submits the commands to tl1_command().
 * It does not pass the semicolon on.
 */
int tl1_parse_line(char *buffer)
{
	int len;

//	printf("Parse: %s\n", buffer);

	/*
	 * Trim leading whitespace
	 */
	while (isspace(*buffer))
	{
		buffer++;
	}

	/*
	 * Trim off trailing whitespace
	 */
	len = strlen(buffer);
	while (isspace(buffer[len - 1]))
	{
		len--;
		buffer[len] = '\0';
	}

	/*
	 * Early exits
	 */
	if (buffer[0] == '\0')
	{
		return 0;
	}

	/*
	 * Merge this portion in with rest of command anready
	 * built up.
	 */
	if (command)
	{
		char *newcommand;

		len = strlen(command) + strlen(buffer) + 1;
		newcommand = malloc(len);
		strcpy(newcommand, command);
		strcat(newcommand, buffer);
		free(command);
		command = newcommand;
	}
	else
	{
		len = strlen(buffer) + 1;
		command = malloc(len);
		strcpy(command, buffer);
	}

	/*
	 * If we have at least one semicolon,
	 * we can process that much of the command.
	 */
	char *pos;
	while((pos = strchr(command, ';')) != NULL)
	{
		*pos = '\n';
		tl1_command(command);
		strcpy(command, pos + 1);
	}

	return 0;
}

