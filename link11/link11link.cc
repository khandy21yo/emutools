//!\file link11link.cc
//! \brief Actual work of linking
//!
//!\author Kevin Handy, Apr 2020
//!

#include "link11.h"


//**********************************************************************
// link11link.cc
//**********************************************************************

//!/brief Initial pass to strip off TXT blocks and global vars
//!
//! This pass is used to strip off the TXT psects, and the global
//! variables, and jenerate the necessary tables.
//!!
//!\returns error code
//!
int Link::PassTxt(ObjectBlock &block)
{
	switch(block.type)
	{
	case BLOCK_GSD:
	case BLOCK_ENDGSD:
	case BLOCK_TXT:
	case BLOCK_RLD:
	case BLOCK_ISD:
	case BLOCK_ENDMOD:
	case BLOCK_LIB:
	case BLOCK_ENDLIB:
		std::cout << "PassTxt: unhandled type " <<
			block.type << std::endl;
		break;
	}

	return ERROR_OK;
}

