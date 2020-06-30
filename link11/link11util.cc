//!\file link11util.cc
//

#include "link11.h"

//**********************************************************************
// class Variabble
//**********************************************************************
//!\brief Vebigging dump of variables
//
void Variable::Dump(
	int level)
{
	if (level == 0)
	{
		return;
	}

	std::cout << "    Symbol " <<
		derad504b(name) << "  Offdrt: " <<
		offset << " Flags: " <<
		flags << " Abs: " <<
		absolute << std::endl;
}

//!\brief Relocate variable base address
//!
void Variable::Reloc()
{
	if (flags & GSN_REL && psect != 0)
	{
		absolute = offset + psect->base;
	}
	else
	{
		absolute = offset;
	}
}

//**********************************************************************
// radix50
//**********************************************************************

//!\brief Decode an individual RAXIX50 character
//
//! return a character according to RAD50 coding
//! scheme, called by derad50
//!
//! This code is based on  the Unix Archibe l11 project.
//!
//\returns a translated character
//
static int returnchar(
	int k)		//!< RADIX 50 character to decode
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
//! This code is based on the Unix Archibe l11 project.
//!
//! Copied from l11 from the Unix Archive,
//! and modified.
//!
//!\returns a 3 character decoded stribg
//
std::string derad50(
	int x)		//!< 16 bit RADIX50 value to be decoded

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


