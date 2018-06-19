/**\filename tl1command.c
 * \brief Process tl1 commands
 *
 * Parses and executes tl1 commands
 *
 * \author Kevin Handy 02/27/2009
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "tl1parse.h"

#define MAX_FIELDS 50


/** \brief Known tl1 commands structure
 *
 * Used to generate a list of known tl1 commands.
 */
struct command_entry
{
	char *command;					/**< tl1 command. Case must match command file name */
	int (*function)(int cmd, int rows, char *fields[]);	/**< Function to call */
	int authrequired;				/**< Must be authorized to use */
};

/** \brief List of tl1 commands
 *
 * This is all of the commands that the tl1 parser understands.
 * The second parameter specifies which internal function handles that command.
 * The function tl1_execute_default calls an external function to handle the command.
 *
 * This list could be generated by looking up all the files in the
 * /bin/tl1 directory, but then it would have to be filtered against
 * a list of exceptions (like '/bin/tl1/functions'), and would allow
 * a possible attack by creating another file there.
 *
 * Built in functions should be much more secure from attack than
 * external commands, so the most sensitive commands should be built in.
 */
struct command_entry command_list[] =
{
	{ "act-user", tl1_execute_actuser, 0 },
	{ "alw-msg-all", tl1_execute_default, 1 },
	{ "break", tl1_execute_break, 0 },
	{ "canc-user", tl1_execute_default, 1 },
	{ "dlt-bind-spn", tl1_execute_default, 1 },
	{ "dlt-ntpassoc", tl1_execute_default, 1 },
	{ "dlt-snmp", tl1_execute_default, 1 },
	{ "dlt-user-secu", tl1_execute_default, 1 },
	{ "ed-bind-spn", tl1_execute_default, 1 },
	{ "ed-ip-static", tl1_execute_default, 1 },
	{ "ed-ntpassoc", tl1_execute_default, 1 },
	{ "ed-pid", tl1_execute_default, 1 },
	{ "ed-resolv", tl1_execute_default, 1 },
	{ "ed-snmp", tl1_execute_default, 1 },
	{ "ed-user-secu", tl1_execute_default, 1 },
	{ "ent-bind-spn", tl1_execute_default, 1 },
	{ "ent-ip-dhcp", tl1_execute_default, 1 },
	{ "ent-ip-static", tl1_execute_default, 1 },
	{ "ent-mngt-pvc", tl1_execute_default, 1 },
	{ "ent-ntpassoc", tl1_execute_default, 1 },
	{ "ent-radius", tl1_execute_default, 1 },
	{ "ent-resolv", tl1_execute_default, 1 },
	{ "ent-snmp", tl1_execute_default, 1 },
	{ "ent-snmp-config", tl1_execute_default, 1 },
	{ "ent-spn-config", tl1_execute_default, 1 },
	{ "ent-syslog", tl1_execute_default, 1 },
	{ "ent-user-secu", tl1_execute_default, 1 },
	{ "init-inid", tl1_execute_default, 1 },
	{ "init-reg-pvc", tl1_execute_default, 1 },
	{ "opr-lpbk-pvc", tl1_execute_default, 1 },
	{ "rls-lpbk-pvc", tl1_execute_default, 1 },
	{ "rtrv-alm-all", tl1_execute_default, 1 },
	{ "rtrv-bind-all", tl1_execute_default, 1 },
	{ "rtrv-bind-spn", tl1_execute_default, 1 },
	{ "rtrv-config", tl1_execute_default, 1 },
	{ "rtrv-hw", tl1_execute_default, 1 },
	{ "rtrv-ip", tl1_execute_default, 1 },
	{ "rtrv-ntp", tl1_execute_default, 1 },
	{ "rtrv-snmp", tl1_execute_default, 1 },
	{ "rtrv-stat-adsl", tl1_execute_default, 1 },
	{ "rtrv-tod", tl1_execute_default, 1 },
	{ "rtrv-user-secu", tl1_execute_default, 1 },
	{ "rtrv-version", tl1_execute_default, 1 },
	{ "set-date", tl1_execute_default, 1 },
	{ "set-mngt", tl1_execute_default, 1 },
	{ "set-sid", tl1_execute_default, 1 },
	{ "vdn-inid", tl1_execute_default, 1 },
	{ "vup-inid", tl1_execute_default, 1 },
	{0, 0, 0}
};

/** \brief Parses one tl1 command
 *
 * Parses a single tl1 command,
 * then hands it off to be executed.
 *
 * It breaks the comand up into the individual fields
 * (like perls command 'split(/:;/, $buffer)').
 * It doesn't care if there is a terminating semicolon or not.
 */
int tl1_command(char *buffer)
{
	int rows = 0;			/* Next available field */
	char *fields[MAX_FIELDS];	/* Broken down fields */
	char *pos;			/* Searvch pointer */
	int len;

//	printf("Processing: %s\n", buffer);

	/*
	 * Strip crap off end of line
	 */
	len = strlen(buffer);
	while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == ';'))
	{
		buffer[len - 1] = '\0';
		len--;
	}

	/*
	 * Ignore empty commands
	 */
	if (strcmp(buffer, "") == 0)
	{
		return 0;
	}

	/*
	 * Break the line into components
	 */
	while ((pos = strchr(buffer, ':')) != NULL)
	{
		char *bfr;

		/*
		 * Trim off leading whitespace
		 */
		while (isspace(*buffer))
		{
			buffer++;
		}

		/*
		 * Copy over one field
		 */
		len = pos - buffer;
		bfr = malloc(len + 1);
		strncpy(bfr, buffer, len);
		bfr[len] = '\0';

		/*
		 * Trim off trailing whitespace
		 */
		while (len > 0 && isspace(bfr[len - 1]))
		{
			bfr[len - 1] = '\0';
			len--;
		}

		/*
		 * Save field, prepare for next one
		 */
		buffer = pos + 1;
		fields[rows++] = bfr;
	}

	/*
	 * Copy over last field
	 */
	{
		/*
		 * Trim off leading whitespace
		 */
		while (isspace(*buffer))
		{
			buffer++;
		}

		/*
		 * Trim off trailing whitespace
		 */
		len = strlen(buffer);
		while (len > 0 && isspace(buffer[len - 1]))
		{
			buffer[len - 1] = '\0';
			len--;
		}

		fields[rows] = malloc(strlen(buffer) + 1);
		strcpy(fields[rows], buffer);
		rows++;
	}

	/*
	 * Execute the command
	 */
	return tl1_execute(rows, fields);
}


/** \brief try to execute a tl1 command.
 */
int tl1_execute(int rows, char *fields[])
{
	int loop = 0;

	/*
	 * Search the list of known commands for the one
	 * that the user entered.
	 *
	 * This uses a simple loop, because with the small
	 * number of possible commands a more complicated
	 * lookup method would ust be complicated without
	 * being much faster.
	 */
	while (command_list[loop].command != 0)
	{
		if (strcasecmp(fields[0], command_list[loop].command) == 0)
		{
			/*
			 * Most commands require authorization before you can
			 * execute them.
			 */
			if ((command_list[loop].authrequired != 0) && (auth == 0))
			{
				tl1_error(TL1_DENY, stderr, rows, fields);
				return TL1_DENY;
			}

			/*
			 * Call the apropriate function for that command
			 */
			return (*command_list[loop].function)(loop, rows, fields);
		}

		loop++;
	}

	/*
	 * If we get here, we couldn't understand the command
	 */
	tl1_error(TL1_IICM, stderr, rows, fields);
#if 0
	tl1_execute_dump(rows, fields);
#endif

	return TL1_IICM;
}

/** \brief Default tl1 command executer
 *
 * Executes externally defined commands.
 * Builds up a command line based on the users input,
 * and submits it for execution.
 */
int tl1_execute_default(int cmd, int rows, char *fields[])
{
	int len = rows + 20;
	int loop;
	char *command;
	int status;

//	printf("Execute: %s\n", fields[0]);

	/*
	 * Calculate a minimum size for the command line to be created
	 */
	for (loop = 0; loop < rows; loop++)
	{
		len += strlen(fields[loop]);
	}

	/*
	 * Generate the command line
	 */
	command = malloc(len);

	strcpy(command, "/bin/tl1/");
	strcat(command, command_list[cmd].command);
	strcat(command, " '");
	for (loop = 0; loop < rows - 1; loop++)
	{
		strcat(command, fields[loop]);
		strcat(command, ":");
	}
	strcat(command, fields[rows - 1]);
	strcat(command, "'");

	/*
	 * Execute the command
	 */
	printf("Test command: \"%s\"\n", command);

	setenv("AUTH", (auth == 0) ? "n" : "y", 1);
	status = system(command);

	switch(status)
	{
	case 0:		// All Ok.
		tl1_ack(TL1_ACK_OK, stderr, rows, fields);
		break;

	default:
		tl1_error(TL1_DENY, stderr, rows, fields);
	}

	return status;
}

/** \brief Activate user, break
 *
 * These commands take a modified set of parameters
 * from the normal tl1 commands format.
 *
 */
int tl1_execute_actuser(int cmd, int rows, char *fields[])
{
//DEBUG: ACT-USER:INID0000001:console:123:bspnlk54tzToxScg

	char command[128];
	int status;

	/*
	 * Check for minimum number of parameters
	 */
	if (rows > 4)
	{
		/*
		 * Build up command line from passed parameters
		 */
		sprintf(command, "/bin/tl1/%s USERID=%s,PASSWORD=%s",
			command_list[cmd].command, fields[2], fields[4]);
	}
	else
	{
		tl1_error(TL1_MISP, stderr, rows, fields);
		return TL1_MISP;
	}

	/*
	 * Execute the command
	 */
	printf("Test auth command: \"%s\"\n", command);

	setenv("AUTH", (auth == 0) ? "n" : "y", 1);
	status = system(command);

	switch(status)
	{
	case 0:		// All Ok.
		tl1_ack(TL1_ACK_OK, stderr, rows, fields);
		auth = 1;
		break;

	default:
		tl1_error(TL1_DENY, stderr, rows, fields);

		/*
		 * Lose autherization, even if they had it before.
		 */
		auth = 0;
	}

	return status;
}

/** \brief break
 *
 * These commands take a modified set of parameters
 * from the normal tl1 commands format.
 *
 */
int tl1_execute_break(int cmd, int rows, char *fields[])
{
	char command[128];
	int status;

	/*
	 * Check for minimum number of parameters
	 */
	if (rows > 4)
	{
		/*
		 * Build up command line from passed parameters
		 */
		sprintf(command, "/bin/tl1/%s USERID=%s,PASSWORD=%s",
			command_list[cmd].command, fields[2], fields[4]);
	}
	else
	{
		tl1_error(TL1_MISP, stderr, rows, fields);
		return TL1_MISP;
	}

	/*
	 * Execute the command
	 */
	printf("Test break command: \"%s\"\n", command);

	setenv("AUTH", (auth == 0) ? "n" : "y", 1);
	status = system(command);

	switch(status)
	{
	case 0:		// All Ok.
		tl1_ack(TL1_ACK_OK, stderr, rows, fields);
		break;

	default:
		tl1_error(TL1_DENY, stderr, rows, fields);
	}

	return status;
}


/** \brief Dump out fields
 *
 * Debugging code to dump out the fields after they have been split up.
 * Used to debug the command line parsing code.
 */
int tl1_execute_dump(int rows, char *fields[])
{
	while(rows)
	{
		printf("  %d: '%s'\n", rows, fields[rows - 1]);
		rows--;
	}

	return 0;
}

