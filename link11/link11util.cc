//!\file link11util.cc
//

#include "link11.h"

//! return a character according to RAD50 coding
//! scheme, called by derad50
//~
static int returnchar(
	int k)
{
	if (k >= 1 && k <= 26)
		/* k represents a letter */
		return('a' + k - 1);

	else if (k >= 30 && k <= 39)
		/* k represents a digit */
		return('0' + k - 30);

	else switch (k)
	{
	case 0:
		return(' ');

	case 27:
		return('$');

	case 28:
		return('.');

	case 29:
		return ('_');

	default:
		//
		// Should never happen
		//
		std::cerr << "RAD50 non-character" << std::endl;
		return 0; /* NOTREACHED */
	}

}



//! decode a word in which 3 characters are coded by 
//! the RAD50 scheme.
//!
//! Copied from l11 from the Unix Archive,
//! and modified.
//
std::string derad50(
	int x)

{
	char s[4];
	s[3] = '\0';
	s[2] = returnchar(x % 40);
	x /= 40;
	s[1] = returnchar(x % 40);
	x /= 40;
	s[0] = returnchar(x % 40);

	return std::string(s);
}


