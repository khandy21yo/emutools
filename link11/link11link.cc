//!\file link11link.cc
//! \brief Actual work of linking
//!
//!\author Kevin Handy, Apr 2020
//!

#include "link11.h"


//**********************************************************************
// link11link.cc
//**********************************************************************

//!\brief Initial pass to strip off TXT blocks and global vars
//!
//! This pass is used to strip off the TXT psects, and the global
//! symbols, and jenerate the necessary tables.
//!!
//!\returns ErrorCode
//!
int Link::PassTxt(
	ObjectBlock &block)	//!< One block of code from an object file
{
	switch(block.type)
	{
	case BLOCK_GSD:
		//
		// In this pass, we are only interested in global symbol
		// definitions, and psect definitions.
		//
		for (int loop = 0; loop < block.length - 6; loop += 8)
		{
			int attr = block.block[loop + 4];

			std::cout <<
				derad504b(block.block + loop) << "  ";

			switch(block.block[loop + 5])
			{
			case GSD_MODNAM:
				std::cout << "module ";
				break;
			case GSD_CSECT:
				std::cout << "csect  ";
				break;
			case GSD_ISN:
				std::cout << "syngol ";
				break;
			case GSD_TA:
				std::cout << "tran   ";
				break;
			case GSD_GSN:
				std::cout << "global ";
				break;
			case GSD_PSECT:
				std::cout << "psect  ";
				break;
			case GSD_IDENT:
				std::cout << "ident  ";
				break;
			case GSD_VSECT:
				std::cout << "vsect  ";
				break;
			default:
				std::cout << "????   ";
				break;
			}

		}
		break;
	case BLOCK_ENDGSD:
	case BLOCK_TXT:
	case BLOCK_RLD:
	case BLOCK_ISD:
	case BLOCK_ENDMOD:
	case BLOCK_LIB:
	case BLOCK_ENDLIB:
		// not yet implemented at all
		std::cout << "PassTxt: unimplemented type " <<
			block.type << std::endl;
		break;
	}

	return ERROR_OK;
}

