/** \file tl1error.c
 *
 * Common error handling routines for the t11 programs.
 */

#include "tl1parse.h"
#include <time.h>
#include <locale.h>
#include <langinfo.h>

/*
 * Local prototypes
 */
static void printerror( FILE *os, char m, char *comp,
	char *descr, int errno, int rows, char *fields[]);

/** \brief acknowledgement message
 *
 * Response message in a much simpler format than
 * the error messages handled by tl1_error().
 */
void tl1_ack(
	int code,	/**< Ack code */
	FILE *os,	/**< Channel to print message to */
	int rows,	/**< Number of parsed fields */
	char *fields[]	/**< Parsed Fields */
)
{
	char *ctag;	/* ctag from original command */
	char *descr;	/* Description based on ack code */

	/*
	 * Find the ctag if available
	 */
	if (fields && (rows > 3))
	{
		ctag = fields[3];
	}
	else
	{
		ctag = "";
	}

	/*
	 * Calculate descr based on code
	 */
	switch(code)
	{
	case TL1_ACK_IP:
		descr = "IP";
		break;

	case TL1_ACK_PF:
		descr = "PF";
		break;

	case TL1_ACK_OK:
		descr = "OK";
		break;

	case TL1_ACK_NA:
		descr = "NA";
		break;

	case TL1_ACK_NG:
		descr = "NG";
		break;

	case TL1_ACK_RL:
		descr = "RL";
		break;

	default:
		descr = "??";
		break;
	};

	/*
	 * Output message
	 */
	fprintf(os, "%s %s\n", descr, ctag);
}

/** \brief Print out an error message
 *
 * Print out an error message based on the error code.
 * The output can be directed to any file,
 * but should normally be sent to stderr.
 *
 * \returns Status code.
 */
void tl1_error(
	int errno,	/**< Error code */
	FILE *os,	/**< Channel to print message to */
	int rows,	/**< Number of parsed fields */
	char *fields[]	/**< Parsed Fields */
)
{
	switch(errno)
	{
	case TL1_IICM:
		printerror(os, 'M', "IICM", "Input, Invalid VERB", errno, rows, fields);
		break;

	case TL1_DENY:
		printerror(os, 'M', "DENY", "Access denied", errno, rows, fields);
		break;

	case TL1_PLNA:
		printerror(os, 'M', "PLNA", "Login Not Active", errno, rows, fields);
		break;

	case TL1_MISP:
		printerror(os, 'M', "MISP", "Missing parameters", errno, rows, fields);
		break;

	default:
		printerror(os, 'M', "ERRX", "Unspecific error", errno, rows, fields);
		break;
	}
}

/** \brief Fancy print for error messages.
 *
 * Given the numerous bits involved in the error messages,
 * print them out in the proper format.
 */
static void printerror(
	FILE *os,		/**< Output stream */
	char m,			/**< Defferentiates between response and autonomous messages */
	char *comp,		/**< Completion code */
	char *descr,		/**< Response block */
	int errno,		/**< Error code for comp/descr */
	int rows,		/**< Number of parsed fields */
	char *fields[]		/**< Parsed Fields */
)
{
	time_t lt;
	struct tm *tm;
	char datestring[40];
	char *ctag;

	/*
	 * Get text version of current date and time
	 */
	setlocale(LC_ALL, "");
	lt = time(0);
	tm = localtime(&lt);
#if 0
	strftime(datestring, sizeof(datestring), nl_langinfo (D_T_FMT), tm);
#else
	strftime(datestring, sizeof(datestring), "%y-%m-%d %T", tm);
#endif

	/*
	 * Find the ctag if available
	 */
	if (fields && (rows > 3))
	{
		ctag = fields[3];
	}
	else
	{
		ctag = "";
	}

	/*
	 * \FIXME Test code. Placeholder.
	 *
	 * Generic error message.
	 */
	fprintf(os, "\n   %s %s\n"
		"%c  %s %s\n"
		"   /* %s */\n"
		"ERROR: %d\n;\n",
		hostname,
		datestring,
		m,
		ctag,
		comp,
		descr,
		errno);
}
