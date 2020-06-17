//!\file link11link.cc
//! \brief Actual work of linking
//!
//!\author Kevin Handy, Apr 2020
//!

#include "link11.h"


//**********************************************************************
// LinkPsect
//**********************************************************************

//!\brief Dump data structures
//!
//! Debugging routine to display data stored in class
//
void LinkPsect::Dump(
	int Level)	//!< Detail level
{
	std::cout <<
		"  psect moduke: " <<
		derad504b(module) << "  psect: " <<
		derad504b(name) << "  flag: " <<
		flag << "  length: " <<
		length << "  base: " <<
		base << std::endl;

}

//**********************************************************************
// Link
//**********************************************************************

//!\brief Dump out data in structure
//!
//! Debugging routine to display data stored in the class.
//!
void Link::Dump(
	int level)	//!< Level of detail
{
	std::cout << std::endl << "Link" << std::endl;
	std::cout << " Current module: " <<
		derad504b(currentmodule) <<
		std::endl;
	psectlist.Dump(level);
}

//!\brief Pass to initialize psect tables
//!
//! Pass to scan through all blocks of data and to set up LinkPsect
//! structures.
//!!
//!\returns ErrorCode
//!
int Link::Pass100(
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

//			std::cout <<
//				derad504b(block.block + loop) << "  ";

			switch(block.block[loop + 5])
			{
			case GSD_MODNAM:
//				std::cout << "module ";
				memcpy(currentmodule, block.block, 4);
				break;
			case GSD_CSECT:
//				std::cout << "csect  ";
				Pass100Psect(block.block[loop + 5],
					block.block + loop);
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
//				std::cout << "psect  ";
				Pass100Psect(block.block[loop + 5],
					block.block + loop);
				break;
			case GSD_IDENT:
				std::cout << "ident  ";
				break;
			case GSD_VSECT:
//				std::cout << "vsect  ";
				Pass100Psect(block.block[loop + 5],
					block.block + loop);
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
		std::cout << "Pass100: unimplemented type " <<
			block.type << std::endl;
		break;
	}

	return ERROR_OK;
}

//!\brief Handle setting up a psect/csect for Link::Pass100.
//!
//! Creates psect entries in the Link structure.
//!
//!\todo It should merge psects together with common names,
//! but this first implementation just adds all psects as
//! new.
//
int Link::Pass100Psect(
	int type,			//!< type of section (pset. csrct)
	const unsigned char *def)	//!< Pointer to definition in GSD
{
	LinkPsect *psect = 0;	//!< Pointer to this new psect

	psect = &(*psectlist.emplace(psectlist.end()));

	memcpy(psect->module, currentmodule, 4);
	memcpy(psect->name, def, 4);
	psect->flag = def[4];
	psect->length = def[6] + (def[7] << 8);

	if (psect->length != 0)
	{
		psect->data = new unsigned char[psect->length];
	}

	return 0;
}

