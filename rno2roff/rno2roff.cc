//! \file rno2roff.cc
//!
//! \breif Filter to convert DSR (Digital Standard Runoff) to roff.
//!
//! This filter will read in a rno file,
//! and convert it into [ntg]roff format.
//!
//! Use it asy, March 2016
//! rno2roff < source > dest
//!
//! \author Kevin Hand


#include <iostream>
#include <string>

#include<cstdlib>
#include <cctype>

//
// Prototypes
//
void rno_filter(std::istream &in);
std::string parse_text(const std::string &src, int &ptr);
std::string parse_dot(const std::string &src, int &ptr);
int search_dot(const std::string &src, int &ptr);
int dot_match(const char *match, const std::string src, int &ptr);


//
// Global values
//
static int uc_flag = 0;		//!< Upper case flag (0 = lc everything,
				//! 1 = retain case.
static int uc1_flag = 0;	//!< Upper case next character (^)?
static int uc2_flag = 0;	//!< Upper case characters (\^)?
static int ul1_flag = 0;	//!< Underline next character?
static int in_footnote = 0;	//!< In footnote?
static int lm;			//!< Left margin position (runoff)
static int rm = 72;		//!< Right margin position (runoff)
static int chapter = 0;		//!< Chapter number
static int debug = 0;		//!< Enable debugging output?
				//!< 0=No, 1=Yes.

//! \brief Main function.
//!
//! Main function.
//! Handles all user options, 
//! and runs filter.
//!
int main(int argc, char **argv)
{
	rno_filter(std::cin);
}

//! \brief Main filter
//!
//! Main filter program
//!
void rno_filter(std::istream &in)
{
	std::string buffer;
	int ptr;

	std::cout <<
		".\\\" Converted from RNO to nroff\n" <<
		".\\\" Uses the -mm macro package\n" <<
		".INITI N rnoindex\n" <<
		".INITR rnoindex\n" <<
	       ".nr Ej 1\n" << std::endl;

	while (getline(in, buffer))
	{
		ptr = 0;
		std::cout << parse_text(buffer, ptr) <<std::endl;
	}
}

//! \brief Translate text line
//!
//! Converts a text line in RNO format to roff.
//!
//! \return filtered string.
//!
std::string parse_text(
	const std::string &src,	//!< String to convert
	int &ptr)		//!< Pointer into string
{
	std::string result;	//!< Result string being built.
	char ch;		//< For looking at characters.
	int firstchar = 1;	//< Are we on the first character?

	//
	// Handle any dot commands
	//
	while (ptr < src.size() && src[ptr] == '.')
	{
		result += parse_dot(src, ptr);
	}

	//
	// Footnote end?
	//
	if (in_footnote != 0 && src[0] == '!')
	{
		result += ".FE\n";
		ptr = src.size();
		in_footnote = 0;
	}

	//
	// Handle ordinary text
	//
	while (ptr < src.size())
	{

		switch (src[ptr])
		{
		// & Ampersand Underscoring
		case '&':
			ul1_flag =1;
			break;

		// # Number Sign Explicit space
		case '#':
			result.append("\\ ");
			break;

		// _ Underline Quote next character
		case '_':
			// roff special characters don't match RNO specials
			ch = src[++ptr];	// hope they aren't lying
			if (ch == '\\')
			{
				result += "\\\\";
			}
			else
			{
				result += ch;
			}
			break;

		// ^ Circumflex Upper-case shift or mode lock
		case '^':
			//
			// Try for mode lock
			//
			ch = src[ptr+1];
			switch(ch)
			{
			case '^':
				uc2_flag = 1;
				ptr++;
				break;
			case '&':
				result += "\\fI";
				ptr++;
				break;

			default:
				uc1_flag = 1;
				break;
			}
			break;

		// \ Backslash Lower-case shift or mode unlock
		case '\\':
			//
			// Try for mode lock
			//
			ch = src[ptr+1];
			switch(ch)
			{
			case '\\':
				uc2_flag = 0;
				ptr++;
				break;
			case '&':
				result += "\\fR";
				ptr++;
				break;
			default:
				uc1_flag = 2;
				break;
			}
			break;
			break;

		// < Less-than Capitalize next word
		case '<':
			result += src[ptr];
			break;

		// = Equals-sign Hypenation disable
		case '=':
			break;

		default:
			ch = src[ptr];

			//
			// Is the file set to .LOWER CASE
			//
			if (uc_flag == 0)
			{
				ch = tolower(ch);
			}
			//
			// Is ther a single upper case flag set (^)?
			//
			if (uc1_flag == 1)
			{
				ch =toupper(ch);
				uc1_flag = 0;
			}
			//
			// Is ther a single lower case flag set (^)?
			//
			if (uc1_flag == 2)
			{
				ch =tolower(ch);
				uc1_flag = 0;
			}
			//
			// Is ther a long upper case flag set (^^)?
			//
			if (uc2_flag == 1)
			{
				ch =toupper(ch);
			}
			//
			// Is ther a long upper case flag set (^^)?
			//
			if (uc2_flag == 2)
			{
				ch =tolower(ch);
			}
			//
			// Did we somehow get a dot as the first character?
			//
			if (firstchar != 0 && ch == '.')
			{
				result += "\\&";
			}
			firstchar = 0;
			if (ul1_flag)
			{
				result += "\\fI";
			}
			result += ch;
			if (ul1_flag)
			{
				result += "\\fR";
				ul1_flag = 0;
			}
			break;
		}

		ptr++;
	}
	if (*(result.rbegin()) == '\n' || *(result.rbegin()) == ' ')
	{
		result.erase(result.size() - 1, 1);
	}
	return result;
}


//! \brief parsing option numbers
//!
//! These numbers are used to handle determining what to do with
//! the dot commands
//!
enum dotopt
{
	OP_COMMENT = 100,	// Comment, eat text to end of line
	OP_EASY,		// Simple command, ends at eol or ;
	OP_DRPPRM,		// Like simple, but drops parameters
	OP_FOOTNOTE,		// Start of a footnote
	OP_FIGURE,		// FIGURE
	OP_HEADER,		// Header level
	OP_INDEX,		// Index entry
	OP_CHAPTER,		// Chapter title
	OP_CASE,		// Upper/lower case flags
	OP_REPEAT,		// Repeat operator
	OP_LEFT,		//LLeft margin
	OP_RIGHT,		// Right margin
	OP_DEBUG,		// Turn on debug outyput
	OP_NODEBUG		// Turn off debug output
};
//! \brief Structure to handle list of RNO commands
//!
//! This structure lists all of the dor commands defined in the
//! RSTS/E RNO manual.
//!
struct rno_commands
{
	const char *text;	//!< RNO command
	int opcode;		//!< Option decode value. 0 = undefined
	const char *value;	//!< Replacement text, 0 = undefined
};

//! \brief Fill in the table
//!
//! Lists all of the commands in the RSTS/E dsr manual
//!
struct rno_commands rnoc[] =
{
	"BREAK", OP_EASY, ".br",	// this oneis ignored, because
					// returning 0 is nomatch.
	"BREAK", OP_EASY, ".br",
	"BR", OP_EASY, ".br",
	"SKIP", OP_EASY, ".sp",
	"S", OP_EASY, ".sp",
	"BLANK", OP_EASY, ".br\n.sp",
	"B", OP_EASY, ".br\n.sp",
	"FIGURE", OP_FIGURE, 0,
	"FG", OP_FIGURE, 0,
	"INDENT", OP_EASY, ".ti",
	"I", OP_EASY, ".ti",
	"PARAGRAPH", OP_EASY, ".P",
	"P", OP_EASY, ".P",
	"CENTER", OP_DRPPRM, ".ce",
	"CENTRE", OP_DRPPRM, ".ce",
	"C", OP_DRPPRM, ".ce",
	"FOOTNOTE", OP_FOOTNOTE, ".FS",
	"FN", OP_FOOTNOTE, ".FS",
	"NOTE", OP_EASY, ".NT",
	"NT", OP_EASY, ".NT",
	"END NOTE", OP_EASY, ".NE",
	"EN", OP_EASY, ".NE",
	"LIST ELEMENT", OP_DRPPRM, ".LI",
	"LIST", OP_DRPPRM, ".AL",
	"LS", OP_DRPPRM, ".AL",
	"LE", OP_DRPPRM, ".LI",
	"END LIST", OP_DRPPRM, ".LE",
	"ELS", OP_DRPPRM, ".LE",
	"COMMENT", OP_COMMENT, ".\\\"",
	"PAGE", OP_EASY, ".bp",
	"PG", OP_EASY, ".bp",
	"TEST PAGE", OP_EASY, ".ne",
	"TP", OP_EASY, ".ne",
	"NUMBER", 0, 0,
	"NM", 0, 0,
	"NUMBER CHAPTER", 0, 0,
	"NUMBER APPENDIX", 0, 0,
	"NONUMBER", 0, 0,
	"NNM", 0, 0,
	"CHAPTER", OP_CHAPTER, 0,
	"CH", OP_CHAPTER, 0,
	"HEADERLEVEL", OP_HEADER, ".H",
	"HL", OP_HEADER, ".H",
	"TITLE", OP_INDEX, ".PH",
	"T", OP_INDEX, ".PH",
	"FIRST TITLE", 0, 0,
	"FT", 0, 0,
	"SUBTITLE", 0, 0,
	"ST", 0, 0,
	"INDEX", OP_INDEX, ".IND",
	"X", OP_INDEX, ".IND",
	"DO INDEX", 0, 0,
	"DX", 0, 0,
	"PRINT INDEX", OP_EASY, ".INDP",
	"PX", OP_EASY, ".INDP",
	"SUBPAGE", 0, 0,
	"END SUBPAGE", 0, 0,
	"APPENDIX", 0, 0,
	"AX", 0, 0,
	"HEADER", 0, 0,
	"HD", 0, 0,
	"NOHEADER", 0, 0,
	"NHD", 0, 0,
	"JUSTIFY", OP_EASY, ".ad b",
	"J", OP_EASY, ".ad b",
	"NOJUSTIFY", OP_EASY, ".na",
	"NJ", OP_EASY, ".na",
	"FILL", OP_EASY, ".fi",
	"F", OP_EASY, ".fi",
	"NOFILL", OP_EASY, ".nf",
	"NF", OP_EASY, ".nf",
	"UPPER CASE", OP_CASE, "U",
	"UC", OP_CASE, "U",
	"LOWER CASE", OP_CASE, 0,
	"LC", OP_CASE, 0,
	"FLAGS CAPITALIZE", 0, 0,
	"FL CAPITALIZE", 0, 0,
	"NO FLAGS CAPITALIZE", 0, 0,
	"NFL", 0, 0,
	"HYPHENATION", OP_EASY, ".hy",
	"HY", OP_EASY, ".hy",
	"NO HYPHENATION", OP_EASY, ".nh",
	"NHY", OP_EASY, ".nh",
	"FLAGS HYPHENATE", 0, 0,
	"PERIOD", 0, 0,
	"PR", 0, 0,
	"NOPERIOD", 0, 0,
	"NPR", 0, 0,
	"LITERAL", OP_DRPPRM, ".VERBON 1",
	"LIT", OP_DRPPRM, ".VERBON 1",
	"END LITERAL", OP_DRPPRM, ".VERBOFF",
	"ELI", OP_DRPPRM, ".VERBOFF",
	"LEFT MARGIN", OP_LEFT, ".po",
	"LM", OP_LEFT, ".po",
	"RIGHT MARGIN", OP_RIGHT, ".ll",
	"RM", OP_RIGHT, ".ll",
	"RIGHT MARGIN", 0, 0,
	"RM", 0, 0,
	"PAPER SIZE", 0, 0,
	"PAGE SIZE", 0, 0,
	"PS", 0, 0,
	"SPACING", OP_EASY, ".ls",
	"SP", OP_EASY, ".ls",
	"STANDARD", 0, 0,
	"SD", 0, 0,
	"TAB STOPS", OP_EASY, ".ta",
	"TS", OP_EASY, ".ta",
	"AUTOPARAGRAPH", 0, 0,
	"AP", 0, 0,
	"NOAUTOPARAGRAPH", 0, 0,
	"NAP", 0, 0,
	"REQUIRE", OP_EASY, ".so",
	"REPEAT", OP_REPEAT, 0,
	"DEBUG", OP_DEBUG, 0,
	"NODEBUG", OP_NODEBUG, 0,
	0, 0, 0
};

//! \brief Translate dot command
//!
//! Converts a dot command to roff command
//!
//! \return filtered string.
//!
std::string parse_dot(
	const std::string &src,	//!< String to convert
	int &ptr)		//!< Pointer into string
{
	std::string result;	//!< Result string being built.
	int cmd;		//!< Used to search for dot command
	std::string partial;	//!< Partial result
	int value;		//!< Temporary value

	if (debug)
	{
		result += std::string(".\\\" ") +
			src + "\n";
	}

	//
	// Skip over the dot
	//
	ptr++;

	//
	// Tryto find match
	//
	cmd = search_dot(src, ptr);

	if (cmd)
	{
		switch(rnoc[cmd].opcode)
		{
		//
		// .COMMENT.
		// Copy everything to end of line
		//
		case OP_COMMENT:
			result += std::string(rnoc[cmd].value) +
				" " + src.substr(ptr) + "\n";
			ptr = src.size();
			break;

		//
		// Left margin opcode
		//
		case OP_LEFT:
			result += std::string(rnoc[cmd].value) + " ";
			partial = "";

			//
			// Copy to eol or ;
			//
			while (ptr < src.size() && src[ptr] != ';' &&
				!(src[ptr] == '.' && src[ptr - 1]== ' '))
			{
				partial += src[ptr];
				result += src[ptr++];
			}
			//
			// Skip whitespace
			//
			while (ptr < src.size() && 
				(src[ptr] == ';' || src[ptr] == ' '))
			{
				ptr++;
			}
			result += "\n";

			value = atoi(partial.c_str());
			if (partial[0] == '+' || partial[0] == '-')
			{
				lm += value;
			}
			else
			{
				lm = value;
			}
			result += ".ll " +
				std::to_string(rm - lm) + "\n";
			if (debug)
			{
				result += ".\\\" LM=" + std::to_string(lm) + 
					", RM=" + std::to_string(rm) + "\n";
			}
			break;

		//
		// Right margin opcode
		//
		case OP_RIGHT:

			//
			// Copy to eol or ;
			//
			while (ptr < src.size() && src[ptr] != ';' &&
				!(src[ptr] == '.' && src[ptr - 1]== ' '))
			{
				partial += src[ptr++];
			}
			//
			// Skip whitespace
			//
			while (ptr < src.size() && 
				(src[ptr] == ';' || src[ptr] == ' '))
			{
				ptr++;
			}
//			result += "\n";

			value = atoi(partial.c_str());
			if (partial[0] == '+' || partial[0] == '-')
			{
				rm += value;
			}
			else
			{
				rm = value;
			}
			result += ".ll " +
				std::to_string(rm - lm) + "\n";
			if (debug)
			{
				result += ".\\\" LM=" + std::to_string(lm) + 
					", RM=" + std::to_string(rm) + "\n";
			}
			break;

		//
		// Easy opcode
		//
		case OP_EASY:
			result += std::string(rnoc[cmd].value) + " ";

			//
			// Copy to eol or ;
			//
			while (ptr < src.size() && src[ptr] != ';' &&
				!(src[ptr] == '.' && src[ptr - 1]== ' '))
			{
				result += src[ptr++];
			}
			//
			// Skip whitespace
			//
			while (ptr < src.size() && 
				(src[ptr] == ';' || src[ptr] == ' '))
			{
				ptr++;
			}
			result += "\n";
			break;

		//
		//
		// Drop parameters
		// For those cases where we don't have any translation
		// for the parameters.
		//
		case OP_DRPPRM:
			result += std::string(rnoc[cmd].value) + "\n";

			//
			// Copy to eol or ;
			//
			while (ptr < src.size() && src[ptr] != ';' && src[ptr] != '.')
			{
				ptr++;
			}
			while (ptr < src.size() && 
				(src[ptr] == ';' || src[ptr] == ' '))
			{
				ptr++;
			}
			break;

		//
		// Copy everything to end of line
		//
		case OP_FOOTNOTE:
			result += std::string(rnoc[cmd].value) + "\n";
			ptr = src.size();
			in_footnote = 1;
			break;

		//
		// FIGURE
		//
		case OP_FIGURE:
			partial = "";

			//
			// Copy to eol or ;
			//
			while (ptr < src.size() && src[ptr] != ';')
			{
				partial += src[ptr++];
			}
			while (ptr < src.size() && 
				(src[ptr] == ';' || src[ptr] == ' '))
			{
				ptr++;
			}
			result += std::string(".ne ") +partial +
				"\n.sp " + partial + "\n";
			break;

		//
		// .HEADER LEVEL
		//
		case OP_HEADER:
			result += std::string(rnoc[cmd].value) + " ";

			//
			// Pick off header level number
			//
			while (ptr < src.size() && src[ptr] == ' ')
			{
				ptr++;
			}
			partial = "";
			while (ptr < src.size() && src[ptr] != ' ')
			{
				partial += src[ptr++];
			}
			result += std::to_string(atoi(partial.c_str()) + 1);
			while (ptr < src.size() && src[ptr] == ' ')
			{
				ptr++;
			}

			//
			// Pull off text
			//
			partial = parse_text(src, ptr);
			result += std::string(" \"") + partial + "\"\n";
			ptr = src.size();
			uc2_flag = 0;
			break;

		//
		// .INDEX
		//
		case OP_INDEX:
			result += std::string(rnoc[cmd].value) + " ";

			while (ptr < src.size() && src[ptr] == ' ')
			{
				ptr++;
			}

			//
			// Pull off text
			//
			partial = parse_text(src, ptr);
			result += std::string(" \"") + partial + "\"\n";
			ptr = src.size();
			break;

		//
		// .CHAPTER
		// nroff doesn't seem to have one of these,so fake it.
		//
		case OP_CHAPTER:
			while (ptr < src.size() && src[ptr] == ' ')
			{
				ptr++;
			}

			//
			// Pull off text
			//
			partial = parse_text(src, ptr);
			chapter++;
//			result +=
//				".bp\n.sp 4\n.ce\nCHAPTER " +
//				std::to_string(chapter) +
//				"\n.sp 1\n.ce\n" +
//				partial +
//				"\n.sp 3\n";
			result +=
				".H 1 \"" + partial + "\"\n"; 
			ptr = src.size();
			uc2_flag = 0;
			break;

		//
		//
		// Upper/Lower case
		//
		case OP_CASE:
			uc_flag = rnoc[cmd].value != 0;

			//
			// Copy to eol or ;
			//
			while (ptr < src.size() && src[ptr] != ';' && src[ptr] != '.')
			{
				ptr++;
			}
			while (ptr < src.size() && 
				(src[ptr] == ';' || src[ptr] == ' '))
			{
				ptr++;
			}
			break;

		//
		// .REPEAT
		//
		case OP_REPEAT:

			//
			// Pick off header level number
			//
			while (ptr < src.size() && src[ptr] == ' ')
			{
				ptr++;
			}
			while (ptr < src.size() && src[ptr] != ' ')
			{
				partial += src[ptr++];
			}
			value = atoi(partial.c_str());
			partial = "";
			while (ptr < src.size() && src[ptr] == ' ')
			{
				ptr++;
			}

			//
			// Pull off text
			//
			while (ptr < src.size() && src[ptr] != ';' &&
				src[ptr] !='.')
			{
				partial += src[ptr++];
			}
			while (ptr < src.size() && 
				(src[ptr] == ';' || src[ptr] == ' '))
			{
				ptr++;
			}
			//
			// Strip off trailing spaces
			//
			while (partial.size() >0 && *(partial.rbegin()) == ' ')
			{
				partial = partial.substr(0, partial.size() - 1);
			}
			//
			// Strip off any quotes (assume they match)
			//
			if (partial[0] == '"')
			{
				partial = partial.substr(1, partial.size() - 2);
			}
			//
			// Marked up string?
			//
			if (partial.size() > 1 && partial[0] == '_')
			{
				partial = partial.substr(1);
			}
			while (value--)
			{
				result += partial;
			}
			result += "\n";
			uc2_flag = 0;
			break;

		case OP_DEBUG:
			debug = 1;
			break;

		case OP_NODEBUG:
			debug = 0;
			break;

		//
		// Unknown command
		//
		default:
			result += std::string(".\\\" Unhandled .") +
				src + "\n";
			ptr = src.size();
			break;
		}
	}
	else
	{
	//
	// If we cannot handle this command
	//
		result += std::string(".\\\" Unknown .") +
			src + "\n";
		ptr = src.size();
	}
	return result;
}

//! \brief Search for dot command in ist
//!
//! Tries to find command in command list.
//!
//! \return Item number in command list.
//!
int search_dot(
	const std::string &src,	//!< String to convert
	int &ptr)		//!< Pointer into string
{
	int cmd;		//!< Loop to scan for a command

	//
	// Scan through possible commands
	//
	for (cmd = 1; rnoc[cmd].text != 0; cmd++)
	{
		if (dot_match(rnoc[cmd].text, src, ptr))
		{
			return cmd;
		}
	}

	//
	// Not found
	//
	return 0;
}


//! \brief try to match a dot command
//!
//! Try to match a simgle dot command.
//! This is made fun because:
//! - Case is not important.
//! - Spaces are not important.
//! - The src string must not continue after match.
//! Assumes that the first character after the dot is non-space.
//!
//! It will adjust ptr if there was a match
//!
//!
//! \return 0 for no match, 1 got match.
//!
int dot_match(const char *match, const std::string src, int &ptr)
{
	int sptr = ptr;		//!< pointer in source text
	int mptr = 0;		//!< pointer to string being matched

	while (sptr <= src.size() && match[mptr] != 0)
	{
		//
		// Point to next non-space source character
		//
		while (sptr < src.size() && src[sptr] == ' ')
		{
			sptr++;
		}

		//
		// Point to next non-space match character
		//
		while (match[mptr] == ' ')
		{
			mptr++;
		}

		//
		// Check characters
		//
		if (match[mptr] != toupper(src[sptr]))
		{
			return 0;
		}

		sptr++;
		mptr++;
	}

	if (match[mptr] != 0)
	{
		//
		// We didn't match to end of match string
		//
		return 0;
	}
	if (sptr < src.size())
	{
		switch (src[sptr])
		{
		case ' ':
			sptr++;
			ptr = sptr;
			return 1;
		case ';':
		case '.':
			return 1;

		default:
			return 0;
		}
	}
	else
	{
		//
		// Matched to the end of the line
		//
		ptr = sptr;
		return 1;
	}
}

