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

	if (currentpsect)
	{
		std::cout << "  Current psect" << std::endl;
		currentpsect->Dump(level);
	}
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

	case BLOCK_TXT:
		Pass100Txt(block);
		break;

	case BLOCK_ENDGSD:
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
	currentpsect = psect;

	memcpy(psect->module, currentmodule, 4);
	memcpy(psect->name, def, 4);
	psect->flag = def[4];
	psect->length = deword(def + 6);
	psect->length += psect->length & 1;	// Force to word boundry size

	if (psect->length != 0)
	{
		//
		// Allocate space for psect.
		// We may need to reallocate later if later psects fold
		// into this one.
		//
		psect->data = new unsigned char[psect->length];
	}

	return 0;
}

//!\brief Pas100 process a txt block
//
int Link::Pass100Txt(
	ObjectBlock &block)	//!< One block of code from an object file
{
	//
	// 1st 2 bytes contain load offset
	//
	unsigned int offset = deword(block.block);

	//
	// The rest of it is object code
	//
	memcpy(currentpsect->data + offset,
		block.block + 2,
		block.length - 8);

	return 0;
}
