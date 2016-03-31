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

#include <cctype>

//
// Prototypes
//
int rno_filter(std::istream &in);
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
int rno_filter(std::istream &in)
{
	std::string buffer;
	int ptr;

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

	//
	// Handle any dor commands
	//
	while (ptr < src.size() && src[ptr] == '.')
	{
		result += parse_dot(src, ptr);
	}

	//
	// Handle ordinary text
	//
	while (ptr < src.size())
	{

		switch (src[ptr])
		{
		// &  Ampersand     Underscoring
		case '&':
			break;

		// #  Number Sign   Explicit space
		case '#':
			result.append("\\ ");
			break;

		// _  Underline     Quote next character
		case '_':
			// roff special characters don't match RNO specials
			ch += src[++ptr];	// hope they aren't lying
			if (ch == '\\')
			{
				result += "\\\\";
			}
			else
			{
				result += ch;
			}
			break;

		// ^  Circumflex    Upper-case shift or mode lock
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
			default:
				uc1_flag = 1;
				break;
			}
			break;

		// \   Backslash    Lower-case shift or mode unlock
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
			default:
				uc1_flag = 2;
				break;
			}
			break;
			break;

		// <  Less-than     Capitalize next word
		case '<':
			result += src[ptr];
			break;

		// =  Equals-sign   Hypenation disable
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
			result += ch;
			break;
		}

		ptr++;
	}
#if 0
	if (*(result.rbegin()) == '\n')
	{
		return result.substr(0, result.size() -1);
	}
	else
	{
		return result;
	}
#else
	if (*(result.rbegin()) == '\n')
	{
		result.erase(result.size() - 1, 1);
	}
	return result;
#endif
}


//! \brief parsing option numbers
//!
//! These numbers are used to handle determining what to do with
//! the dot commands
//!
enum dotopt
{
	OP_COMMENT = 100
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
	"BREAK", 0, 0,
	"BR", 0, 0,
	"SKIP", 0, 0,
	"S", 0, 0,
	"BLANK", 0, 0,
	"B", 0, 0,
	"FIGURE", 0, 0,
	"FG", 0, 0,
	"INDENT", 0, 0,
	"I", 0, 0,
	"PARAGRAPH", 0, 0,
	"P", 0, 0,
	"CENTER", 0, 0,
	"CENTRE", 0, 0,
	"C", 0, 0,
	"FOOTNOTE", 0, 0,
	"FN", 0, 0,
	"NOTE", 0, 0,
	"NT", 0, 0,
	"END NOTE", 0, 0,
	"EN", 0, 0,
	"LIST", 0, 0,
	"LS", 0, 0,
	"LIST ELEMENT", 0, 0,
	"LE", 0, 0,
	"END LIST", 0, 0,
	"ELS", 0, 0,
	"COMMENT", OP_COMMENT, ".\\\"",
	"PAGE", 0, 0,
	"PG", 0, 0,
	"TEST PAGE", 0, 0,
	"TP", 0, 0,
	"NUMBER", 0, 0,
	"NM", 0, 0,
	"NONUMBER", 0, 0,
	"NNM", 0, 0,
	"CHAPTER", 0, 0,
	"CH", 0, 0,
	"NUMBER CHAPTER", 0, 0,
	"HEADERLEVEL", 0, 0,
	"HL", 0, 0,
	"TITLE", 0, 0,
	"T", 0, 0,
	"FIRST TITLE", 0, 0,
	"FT", 0, 0,
	"SUBTITLE", 0, 0,
	"ST", 0, 0,
	"INDEX", 0, 0,
	"X", 0, 0,
	"DO INDEX", 0, 0,
	"DX", 0, 0,
	"PRINT INDEX", 0, 0,
	"PX", 0, 0,
	"SUBPAGE", 0, 0,
	"END SUBPAGE", 0, 0,
	"APPENDIX", 0, 0,
	"AX", 0, 0,
	"NUMBER APPENDIX a", 0, 0,
	"HEADER arg", 0, 0,
	"HD", 0, 0,
	"NOHEADER", 0, 0,
	"NHD", 0, 0,
	"JUSTIFY", 0, 0,
	"J", 0, 0,
	"NOJUSTIFY", 0, 0,
	"NJ", 0, 0,
	"FILL", 0, 0,
	"F", 0, 0,
	"NOFILL", 0, 0,
	"NF", 0, 0,
	"UPPER CASE", 0, 0,
	"UC", 0, 0,
	"LOWER CASE", 0, 0,
	"LC", 0, 0,
	"FLAGS CAPITALIZE", 0, 0,
	"FL CAPITALIZE", 0, 0,
	"NO FLAGS CAPITALIZE", 0, 0,
	"NFL", 0, 0,
	"HYPHENATION", 0, 0,
	"HY", 0, 0,
	"NO HYPHENATION", 0, 0,
	"NHY", 0, 0,
	"FLAGS HYPHENATE", 0, 0,
	"PERIOD", 0, 0,
	"PR", 0, 0,
	"NOPERIOD", 0, 0,
	"NPR", 0, 0,
	"LITERAL", 0, 0,
	"LIT", 0, 0,
	"END LITERAL", 0, 0,
	"ELI", 0, 0,
	"LEFT MARGIN", 0, 0,
	"LM", 0, 0,
	"RIGHT MARGIN", 0, 0,
	"RM", 0, 0,
	"PAPER SIZE", 0, 0,
	"PAGE SIZE", 0, 0,
	"PS", 0, 0,
	"SPACING", 0, 0,
	"SP", 0, 0,
	"STANDARD", 0, 0,
	"SD", 0, 0,
	"TAB STOPS", 0, 0,
	"TS", 0, 0,
	"AUTOPARAGRAPH", 0, 0,
	"AP", 0, 0,
	"NOAUTOPARAGRAPH", 0, 0,
	"NAP", 0, 0,
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
			result =  std::string(rnoc[cmd].value) +
				" " + src.substr(ptr)  + "\n";
			ptr = src.size();
			return result;
		//
		// Unknown command
		//
		default:
			result =  std::string(".\\\" Unhandled .") +
				rnoc[cmd].text + " " + src.substr(ptr)  + "\n";
			ptr = src.size();
			return result;
		}
	}
	else
	{
	//
	// If we cannot handle this command
	//
		result =  std::string(".\\\" Unknown .") + src.substr(ptr)  + "\n";
		ptr = src.size();
		return result;
	}
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
	for (cmd = 0; rnoc[cmd].text != 0; cmd++)
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

